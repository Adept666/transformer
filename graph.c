//Модуль построения графиков
//v0.001.
#include <gtk/gtk.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

//Шрифт:
struct font {
	//Имя:
	char *name;
	//Размер в типографских пунктах:
	double size;
	//Наклон:
	cairo_font_slant_t slant;
	//Толщина (насыщенность):
	cairo_font_weight_t weight;
};

//Цвет:
struct color {
	double r;
	double g;
	double b;
};

//Внешний вид графиков:
struct graph_appearance {
	//Цвет фона:
	struct color background_color;
	//Толщина основных линий сетки:
	double major_gridlines_thickness;
	//Цвет основных линий сетки:
	struct color major_gridlines_color;
	//Толщина промежуточных линий сетки:
	double minor_gridlines_thickness;
	//Цвет промежуточных линий сетки:
	struct color minor_gridlines_color;
	//Шрифт меток осей:
	struct font tick_labels_font;
	//Цвет меток осей:
	struct color tick_labels_color;
	//Шрифт подписей осей:
	struct font axis_labels_font;
	//Цвет подписей осей:
	struct color axis_labels_color;
	//Толщина трасс:
	double traces_thickness;
	//Цвет трасс:
	struct color traces_color;
	//Расстояние между границами виджета и подписями осей в пикселях:
	guint distance_widget_borders_to_axis_labels;
	//Расстояние между подписями осей и метками осей в пикселях:
	guint distance_axis_labels_to_tick_labels;
	//Расстояние между метками осей и линиями сетки в пикселях:
	guint distance_tick_labels_to_gridlines;
};

//!!!ГРАНИЦА ЧИСТОВОГО И ЧЕРНОВОГО КОДА:
//С помощью callback функции не удаётся передать больше одной переменной. Придётся использовать глобальные переменные для хранения данных
//С РАЗНЫМИ КОЛБЕКАМИ ВСЁ РАБОТАЕТ НОРМАЛЬНО!!! ДАЖЕ НЕТ ОШИБКИ GDK
//НИЧЕГО НЕ ПОЛУЧАЕТСЯ! НУЖНО ЛИБО СДЕЛАТЬ КУЧУ КОЛБЭКОВ И ОГРАНИЧИТЬ КОЛИЧЕСТВО drawingarea
//ЛИБО ПРОПИСЫВАТЬ КОЛБЕКИ для каждого drowing area В mainе, а внутри них вызывать функцию, прописанную в модуле
//Аргументом данной функции будет номер графика. Возможно удастся обойтись одной поверхностью и одним контекстом
//!!!!!!!ПРОГРАММА ПАДАЕТ ЕСЛИ НАЖАТЬ НА КНОПКУ И ПОТОМ ПЕРЕЙТИ НА ДРУГОЕ ОКНО - решено с помощью хайда
//!!!Очищать память можно не при закрытии окна, а при закрытии виджета Drawing_area
//Нужно добавить операции уничтожения одного графика (вместе с его строками)
//РУССКИЕ СИМВОЛЫ СОСТОЯТ ИЗ ДВУХ CHAR!!!
//ASCII коды символов, составляющих русские буквы, ОТРИЦАТЕЛЬНЫ!!!
//ПРОБЛЕМА С SEGFAULT ИЗ-ЗА ДИНАМИЧЕСКОГО МАССИВА СИМВОЛОВ РЕШЕНА ЗАМЕНОЙ realloc на free и malloc

//Шкала:
enum scale
{
	//Линейная:
	LIN,
	//Логарифмическая:
	LOG
};

//Геометрия элемента графика:
struct geometry {
	guint y;
	guint x;
	guint height;
	guint width;
};

//Данные, необходимые для построения графика:
struct graph {
	//Шкала оси Y:
	enum scale y_scale;
	//Шкала оси X:
	enum scale x_scale;
	//Количество основных сеток оси Y:
	unsigned int y_number_of_major_grids;
	//Количество основных сеток оси X:
	unsigned int x_number_of_major_grids;
	//Количество промежуточных сеток оси Y:	//!!!
	unsigned int y_number_of_minor_grids;
	//Количество промежуточных сеток оси X:	//!!!
	unsigned int x_number_of_minor_grids;
	void *y_tick_labels;
	void *x_tick_labels;	
	char *y_axis_label;
	char *x_axis_label;
};

//Расширенные данные, необходимые для построения графика (для внутреннего использования):
struct graph_internal {
	unsigned short int number;
	GtkDrawingArea *area;
	gint area_height_request;
	gint area_width_request;
	cairo_surface_t *surface;
	cairo_t *context;
	struct graph graph;
};

//Динамический массив графиков:
struct graph_internal *graph_array;

//Текущее количество запомненных графиков:
unsigned short int number_of_graphs = 0;

//Динамический массив символов:
//char *char_array = "\0\0";	//Почему-то при этом выделяется 4 байта
//char *char_array = "";	//Почему-то при этом выделяется 4 байта
char *char_array;	//Почему-то при этом выделяется 4 байта
//char *char_array = "123456";	//Почему-то при этом выделяется 4 байта
//Наверное придётся использовать first_call

//Текущее количество запомненных строк:
//unsigned short int number_of_strings = 2;
unsigned int number_of_strings = 0;
unsigned int number_of_chars = 0;

//Текущие настройки внешнего вида графиков:
struct graph_appearance graph_appearance;

//Переменная, равная TRUE только при первом вызове функции plot_graph:
static bool first_call = TRUE;

//Объявление функций:
static void graph_clear (unsigned short int graph_number);
static void graph_plot (unsigned short int graph_number, guint area_height, guint area_width);

static void
configure_event_cb (GtkWidget *drawing_area, GdkEventConfigure *event, gpointer data)
{
	g_print("Начало configure\n");
	unsigned short int n;
	memcpy(&n, data, sizeof (unsigned short int));
	if (graph_array[n].surface)
		cairo_surface_destroy (graph_array[n].surface);
	graph_array[n].surface = gdk_window_create_similar_surface (gtk_widget_get_window (GTK_WIDGET (graph_array[n].area)),
																CAIRO_CONTENT_COLOR,
																gtk_widget_get_allocated_width (GTK_WIDGET (graph_array[n].area)),
																gtk_widget_get_allocated_height (GTK_WIDGET (graph_array[n].area)));
	//Очистка поверхности:
	graph_clear (n);
}

static void
draw_cb (GtkWidget *drawing_area, cairo_t *context, gpointer data)
{
	g_print("Начало draw\n");
	unsigned short int n;
	memcpy(&n, data, sizeof (unsigned short int));
	cairo_set_source_surface (graph_array[n].context, graph_array[n].surface, 0, 0);
	cairo_paint (graph_array[n].context);
	g_print("Конец draw\n");
}

static void
size_allocate_cb (GtkWidget *drawing_area, GdkRectangle *allocation, gpointer data)
{
	g_print("Начало size_allocate\n");
	unsigned short int n;
	memcpy(&n, data, sizeof (unsigned short int));
	guint area_height, area_width;
	area_height = gtk_widget_get_allocated_height (GTK_WIDGET (graph_array[n].area));
	area_width = gtk_widget_get_allocated_width (GTK_WIDGET (graph_array[n].area));
	graph_plot (n, area_height, area_width);
	g_print("Конец size_allocate\n");
}

//!!!Тип операции со строками:
enum op_type
{
	//Обновление или первоначальная установка шрифта:
	FONT,
	//Обновление графика:
	GRAPH_UPDATE,
	//Создание графика:
	GRAPH_ADD,
	//Ещё должно быть удаление графика:
	GRAPH_DEL
};













//!!!Выделение памяти под строки:
//!!!!!!!!!!!!!! СОВМЕСТНАЯ РАБОТА FONT и GRAPH_ADD приводят к Core Dumped во время gtk_widget_show_all
static void
string_memory_allocate_FONT ()
{
//Выделение памяти под строки для шрифта при первом вызове
if (number_of_strings == 0)
{
g_print ("\tПервый вызов. Выделение 2 байт памяти\n");
	char_array = (char *) malloc (2 * sizeof (char));	//без этого core dumped
	char_array[0] = '\0';
	char_array[1] = '\0';
	number_of_strings = 2;
	number_of_chars = 2;
}
			g_print ("Операция FONT\n");
			//Создание/обновление названия шрифтов tick и axis:
			//Вычисляю кол-во символов (с учётом 0), занимаемых СЕЙЧАС двумя названиями шрифтов
			//На основе переменной number_of_strings узнаю, сколько строк нужно сохранить для графиков
			//Сохраняю нужные строки во временный массив (символов?)
			//Выделяю новое количество памяти
			//В первые строки пишу полученные имена шрифтов
			//В последние строки пишу данные для графиков из временного массива
			//Удаляю временные массив
			//Обновляю number_of_strings и number_of_chars
			//Обновляю все графики и внешний вид, так как там теперь будут новые адреса

//!!!Проверяем старое содержимое массива:
g_print ("СТАРОЕ СОДЕРЖИМОЕ МАССИВА:\n");
for (int i = 0; i < number_of_chars; i++)
{
	g_print ("\tЭлемент №%i; ", i);
	if (char_array[i] == '\0')
	{
		g_print ("Символ конца строки");
	} else {
		g_print ("Символ: %c", char_array[i]);
	}
	g_print ("; ASCII №%i;\n", char_array[i]);
}
g_print ("КОНЕЦ СТАРОГО МАССИВА:\n");

//Получение требуемого размера массива char временного хранения:
unsigned int AD_font1;	//Старый адрес первого шрифта
unsigned int AD_font2;	//Старый адрес второго шрифта
unsigned int LEN_font1;
unsigned int LEN_font2;

AD_font1 = (unsigned int) char_array;
LEN_font1 = strlen ((char *) AD_font1) + 1;	//Старая Длина с учётом нуль символа
g_print ("\tСтарый адрес первого шрифта: %x\n", AD_font1);
g_print ("\tСтарая длина первого шрифта: %i\n", LEN_font1);

AD_font2 = AD_font1 + LEN_font1 * sizeof (char);
LEN_font2 = strlen ((char *) AD_font2) + 1;	//Старая Длина с учётом нуль символа
g_print ("\tСтарый адрес второго шрифта: %x\n", AD_font2);
g_print ("\tСтарая длина второго шрифта: %i\n", LEN_font2);

unsigned int fonts_len_old = LEN_font1 + LEN_font2;
g_print ("\tСтарая длина шрифтов: %i\n", fonts_len_old);

//Количество символов, которые не изменятся
unsigned int number_of_chars_to_save;
number_of_chars_to_save = number_of_chars - fonts_len_old;
g_print ("\tКоличество символов, которые останутся без изменения: %i\n", number_of_chars_to_save);

char *bufarray_upper;

//Выделение памяти под массив временного хранения:
if (number_of_chars_to_save != 0) {
	bufarray_upper = (char *) malloc (number_of_chars_to_save * sizeof (char));
	g_print ("\tВыделено %i байт памяти под массив временного хранения\n", number_of_chars_to_save * sizeof (char));
	//Сохранение нужных строк во временный массив
	memcpy (bufarray_upper, (char *) (AD_font2 + LEN_font2 * sizeof (char)), number_of_chars_to_save * sizeof (char));
	g_print ("\tВо временный массив скопировано %i символов начиная с адреса %x\n", number_of_chars_to_save * sizeof (char), (AD_font2 + LEN_font2 * sizeof (char)));
} else {
	g_print ("\tМассив временного хранения НЕ СОЗДАЁТСЯ\n");
}

//ПРИНИМАЕМЫЕ адреса и длины шрифтов:
//ЕСЛИ ПЕРЕДАЮТСЯ ОДИНАКОВЫЕ ИМЕНА ШРИФТОВ, ТО ИХ АДРЕСА БУДУТ ОДИНАКОВЫ
AD_font1 = (unsigned int) graph_appearance.tick_labels_font.name;
LEN_font1 = strlen ((char *) AD_font1) + 1;	//Принимаемая Длина с учётом нуль символа
g_print ("\tПринимаемый адрес первого шрифта: %x\n", AD_font1);
g_print ("\tПринимаемая длина первого шрифта: %i\n", LEN_font1);

AD_font2 = (unsigned int) graph_appearance.axis_labels_font.name;
LEN_font2 = strlen ((char *) AD_font2) + 1;	//Принимаемая Длина с учётом нуль символа
g_print ("\tПринимамый адрес второго шрифта: %x\n", AD_font2);
g_print ("\tПринимаемая длина второго шрифта: %i\n", LEN_font2);

unsigned int fonts_len_new = LEN_font1 + LEN_font2;
g_print ("\tНовая длина шрифтов: %i\n", fonts_len_new);

//Перевыделение памяти:
//ЕСЛИ size = 0 - память освобождается. Этого не должно быть
/*if ((fonts_len_new - fonts_len_old) != 0)
{
	char_array = (char *) realloc (char_array, (fonts_len_new - fonts_len_old) * sizeof (char));
	g_print ("\tДобавлено %i байт памяти под массив char_array\n", (fonts_len_new - fonts_len_old) * sizeof (char));
} else {
	g_print ("\tПеревыделение НЕ ТРЕБУЕТСЯ\n");
}*/

free (char_array);
char_array = (char *) malloc ((fonts_len_new + number_of_chars_to_save) * sizeof(char));


//НОВЫЕ адреса шрифтов:
unsigned int AD_font1_new;
unsigned int AD_font2_new;
AD_font1_new = (unsigned int) char_array;
AD_font2_new = AD_font1_new + LEN_font1 * sizeof (char);
g_print ("\tНовый адрес первого шрифта: %x\n", AD_font1_new);
g_print ("\tНовый адрес второго шрифта: %x\n", AD_font2_new);

g_print ("\tКопирование имён шрифтов:\n");
//В первые строки пишу полученные имена шрифтов:
memcpy ((char *) AD_font1_new, (char *) AD_font1, LEN_font1 * sizeof (char));
g_print ("\tСкопировано %i байт из %x в %x\n", LEN_font1 * sizeof (char), AD_font1, AD_font1_new);
memcpy ((char *) AD_font2_new, (char *) AD_font2, LEN_font2 * sizeof (char));
g_print ("\tСкопировано %i байт из %x в %x\n", LEN_font2 * sizeof (char), AD_font2, AD_font2_new);

if (number_of_chars_to_save != 0) {
	g_print ("\tВосстановление текстов для графиков из временного массива:\n");

	//В последние строки пишу данные для графиков из временного массива:
	memcpy ((char *) (AD_font2_new + LEN_font2 * sizeof (char)), bufarray_upper, number_of_chars_to_save * sizeof (char));
	g_print ("\tСкопировано %i байт из %x в %x\n", number_of_chars_to_save * sizeof (char), (unsigned int) bufarray_upper, (AD_font2_new + LEN_font2 * sizeof (char)));

	//Освобождение памяти выделенной массиву временного хранения:
	free (bufarray_upper);
	g_print ("\tМассив временного хранения удалён\n");
}

//Обновляю number_of_strings и number_of_chars:
//number_of strings остаётся без измененй
g_print ("\tДо обновления number_of_chars: %i\n", number_of_chars);
number_of_chars = number_of_chars_to_save + fonts_len_new;
g_print ("\tПосле обновления number_of_chars: %i\n", number_of_chars);

//Обновляю все графики и шрифты, так как адреса теперь новые
//Шрифты:
g_print ("\tОбновление адресов шрифтов:\n");
g_print ("\tАдрес 1-го шрифта до обновления: %x\n", (unsigned int) graph_appearance.tick_labels_font.name);
graph_appearance.tick_labels_font.name = (char *) AD_font1_new;
g_print ("\tАдрес 1-го шрифта после обновления: %x\n", (unsigned int) graph_appearance.tick_labels_font.name);
g_print ("\tАдрес 2-го шрифта до обновления: %x\n", (unsigned int) graph_appearance.axis_labels_font.name);
graph_appearance.axis_labels_font.name = (char *) AD_font2_new;
g_print ("\tАдрес 2-го шрифта после обновления: %x\n", (unsigned int) graph_appearance.axis_labels_font.name);

//К этому моменту количество графиков уже обновлено
g_print ("\tОбновление адресов текстов графиков:\n");
unsigned int ADR1;
ADR1 = AD_font2_new + LEN_font2 * sizeof (char);	//Адрес y_ticks нулевого графика
if (number_of_graphs)
{
	for (unsigned int graph_number = 0; graph_number <= number_of_graphs - 1; graph_number++)
	{
	//Этот цикл не проверялся
		unsigned int LEN = 0;

		graph_array[graph_number].graph.y_tick_labels = (void *) ADR1;
		for (unsigned int i = 1; i <= graph_array[graph_number].graph.y_number_of_major_grids + 1; i++)
		{
			LEN = strlen ((char *) ADR1) + 1;
			ADR1 += LEN * sizeof (char);
		}
		graph_array[graph_number].graph.x_tick_labels = (void *) ADR1;
		for (unsigned int i = 1; i <= graph_array[graph_number].graph.x_number_of_major_grids + 1; i++)
		{
			LEN = strlen ((char *) ADR1) + 1;
			ADR1 += LEN * sizeof (char);
		}
		graph_array[graph_number].graph.y_axis_label = (void *) ADR1;
			LEN = strlen ((char *) ADR1) + 1;	
			ADR1 += LEN * sizeof (char);
		graph_array[graph_number].graph.x_axis_label = (void *) ADR1;
			LEN = strlen ((char *) ADR1) + 1;	
			ADR1 += LEN * sizeof (char);
	}
}
g_print ("\tОбновление адресов текстов графиков завершено:\n");
//Обновление графиков завершено

//!!!Проверяем содержимое массива:
g_print ("СОДЕРЖИМОЕ МАССИВА:\n");
for (int i = 0; i < number_of_chars; i++)
{
	g_print ("\tЭлемент №%i; ", i);
	if (char_array[i] == '\0')
	{
		g_print ("Символ конца строки");
	} else {
		g_print ("Символ: %c", char_array[i]);
	}
	g_print ("; ASCII №%i;\n", char_array[i]);
}
g_print ("КОНЕЦ МАССИВА:\n");
}











static void
string_memory_allocate_GRAPH_ADD ()
{
	//Выделение памяти под строки для шрифта при первом вызове
	if (number_of_strings == 0)
	{
		g_print ("\tПервый вызов. Выделение 2 байт памяти\n");
		char_array = (char *) malloc (2 * sizeof (char));	//без этого core dumped
		char_array[0] = '\0';
		char_array[1] = '\0';
		number_of_strings = 2;
		number_of_chars = 2;
	}

	g_print ("Операция GRAPH_ADD\n");
	//Создание графика:
	//Вычисляю дополнительное количество символов, которое нужно выделить
	//Перевыделяю память (всё старое сохраняется)
	//Пишу принятые строки в новый массив
	//Обновляю number_of_strings
	//Обновляю все графики и внешний вид, так как там теперь будут новые адреса

//Получение адресов начал всех y_number_of_grids + 1 строк:
unsigned int AD_y_ticks;	//Старый адрес начала y_tick_labels;
unsigned int LEN_y_ticks = 0;
unsigned int AD_x_ticks;	//Старый адрес начала x_tick_labels;
unsigned int LEN_x_ticks = 0;
unsigned int AD_y_axis;	//Старый адрес начала y_axis_label;
unsigned int LEN_y_axis = 0;
unsigned int AD_x_axis;	//Старый адрес начала x_axis_label;
unsigned int LEN_x_axis = 0;
g_print ("ВЫЧИСЛЕННЫЕ АДРЕСА:\n");
g_print ("Y TICK LABELS:\n");
unsigned int addr;
unsigned int len;
unsigned int total_len = 0;

for (unsigned int i = 0; i <= graph_array[number_of_graphs - 1].graph.y_number_of_major_grids; i++)
{
	g_print ("\tИтерация: %i\n", i);
	memcpy (&addr, graph_array[number_of_graphs - 1].graph.y_tick_labels + i * sizeof (unsigned int), sizeof (unsigned int));
	g_print ("\tАдрес начала строки: %x\n", addr);
	len = strlen ((char *) addr) + 1;	//Длина с учётом нуль символа
	g_print ("\tДлина строки: %i\n", len);
	LEN_y_ticks += len;
}
	memcpy (&AD_y_ticks, graph_array[number_of_graphs - 1].graph.y_tick_labels, sizeof (unsigned int));
	g_print ("\tАдрес начала Y_ticks: %x\n", AD_y_ticks);
	g_print ("\tДлина Y_ticks: %i\n", LEN_y_ticks);

//Получение адресов начал всех x_number_of_grids + 1 строк:
g_print ("X TICK LABELS:\n");
for (unsigned int i = 0; i <= graph_array[number_of_graphs - 1].graph.x_number_of_major_grids; i++)
{
	g_print ("\tИтерация: %i\n", i);
	memcpy (&addr, graph_array[number_of_graphs - 1].graph.x_tick_labels + i * sizeof (unsigned int), sizeof (unsigned int));
	g_print ("\tАдрес начала строки: %x\n", addr);
	len = strlen ((char *) addr) + 1;	//Длина с учётом нуль символа
	g_print ("\tДлина строки: %i\n", len);
	LEN_x_ticks += len;
}
	memcpy (&AD_x_ticks, graph_array[number_of_graphs - 1].graph.x_tick_labels, sizeof (unsigned int));
	g_print ("\tАдрес начала X_ticks: %x\n", AD_x_ticks);
	g_print ("\tДлина X_ticks: %i\n", LEN_x_ticks);

//Получение адреса начала y_axis_label строки:
g_print ("Y AXIS LABEL:\n");
	memcpy (&AD_y_axis, &graph_array[number_of_graphs - 1].graph.y_axis_label, sizeof (unsigned int));
	g_print ("\tАдрес начала Y_axis: %x\n", AD_y_axis);
	LEN_y_axis = strlen ((char *) AD_y_axis) + 1;	//Длина с учётом нуль символа
	g_print ("\tДлина Y_axis: %i\n", LEN_y_axis);

//Получение адреса начала x_axis_label строки:
g_print ("X AXIS LABEL:\n");
	memcpy (&AD_x_axis, &graph_array[number_of_graphs - 1].graph.x_axis_label, sizeof (unsigned int));
	g_print ("\tАдрес начала X_axis: %x\n", AD_x_axis);
	LEN_x_axis = strlen ((char *) AD_x_axis) + 1;	//Длина с учётом нуль символа
	g_print ("\tДлина X_axis: %i\n", LEN_x_axis);

//Вычисление и суммирование протяжённостей всех строк:
total_len = LEN_y_ticks + LEN_x_ticks + LEN_y_axis + LEN_x_axis;
g_print ("Суммарная протяжённость новых строк: %i\n", total_len);	//С учётом нуль символов	//СХОДИТСЯ

//Выделение нового количества памяти:
/*if ((total_len) != 0)
{
	char_array = (char *) realloc (char_array, total_len * sizeof (char));
	g_print ("\tДобавлено %i байт памяти под массив char_array\n", total_len * sizeof (char));
} else {
	g_print ("\tПеревыделение НЕ ТРЕБУЕТСЯ\n");
}*/

char *bufarray_lower;

//Выделение памяти под массив временного хранения:
	bufarray_lower = (char *) malloc (number_of_chars * sizeof (char));
	g_print ("\tВыделено %i байт памяти под массив временного хранения\n", number_of_chars * sizeof (char));
	//Сохранение нужных строк во временный массив
	memcpy (bufarray_lower, char_array, number_of_chars * sizeof (char));

	free (char_array);
	char_array = (char *) malloc ((number_of_chars+total_len) * sizeof (char));

memcpy (char_array, bufarray_lower, number_of_chars * sizeof (char));

free (bufarray_lower);

//g_print ("Новое количество памяти: %i байт\n", sizeof (char_array));//Херня - вычисляется память под указатель

//Запись новых строк в массив:
//Нужно получить новые адреса в новом массиве
//И скопиравать из старых адресов в новые
unsigned int ADN_y_ticks;	//Новый адрес начала y_tick_labels;
unsigned int ADN_x_ticks;	//Новый адрес начала x_tick_labels;
unsigned int ADN_y_axis;	//Новый адрес начала y_axis_label;
unsigned int ADN_x_axis;	//Новый адрес начала x_axis_label;

//Известно количество строк. Зная его можно вычислить текущее занятое количество символов.
//И писать в адрес следующего за ним символа
//Можно запоминать как number_of_strings так и number_of_chars

	//АКТУАЛЬНО ТОЛЬКО ЕСЛИ СОЗДАЁТСЯ ПЕРВЫЙ ГРАФИК:
//Для получения первого адреса нужно знать длину названий шрифтов:
unsigned int LEN_font1t;	//Объявлено в топе
unsigned int LEN_font2t;	//Объявлено в топе

unsigned int ADN_font1;	//Новый адрес первого шрифта
unsigned int ADN_font2;	//Новый адрес второго шрифта

ADN_font1 = (unsigned int) char_array;
LEN_font1t = strlen ((char *) ADN_font1) + 1;	//Длина с учётом нуль символа
g_print ("\tАдрес первого шрифта: %x\n", ADN_font1);
g_print ("\tДлина первого шрифта: %i\n", LEN_font1t);

ADN_font2 = ADN_font1 + LEN_font1t * sizeof (char);
LEN_font2t = strlen ((char *) ADN_font2) + 1;	//Длина с учётом нуль символа
g_print ("\tАдрес второго шрифта: %x\n", ADN_font2);
g_print ("\tДлина второго шрифта: %i\n", LEN_font2t);

ADN_y_ticks = ADN_font1 + number_of_chars * sizeof (char);
g_print ("\tАдрес Y TICKS: %x\n", ADN_y_ticks);
ADN_x_ticks = ADN_y_ticks + LEN_y_ticks * sizeof (char);
g_print ("\tАдрес X TICKS: %x\n", ADN_x_ticks);
ADN_y_axis = ADN_x_ticks + LEN_x_ticks * sizeof (char);
g_print ("\tАдрес Y AXIS: %x\n", ADN_y_axis);
ADN_x_axis = ADN_y_axis + LEN_y_axis * sizeof (char);
g_print ("\tАдрес X AXIS: %x\n", ADN_x_axis);

memcpy ((char *) ADN_y_ticks, (char *) AD_y_ticks, LEN_y_ticks * sizeof (char));
memcpy ((char *) ADN_x_ticks, (char *) AD_x_ticks, LEN_x_ticks * sizeof (char));
memcpy ((char *) ADN_y_axis, (char *) AD_y_axis, LEN_y_axis * sizeof (char));
memcpy ((char *) ADN_x_axis, (char *) AD_x_axis, LEN_x_axis * sizeof (char));

g_print ("\tnumber_of_strings до обновления: %i\n", number_of_strings);
g_print ("\tnumber_of_chars до обновления: %i\n", number_of_chars);
//Обновляю number_of_strings (возможно эта переменная не понадобится)
number_of_strings += 	graph_array[number_of_graphs - 1].graph.y_number_of_major_grids + 1 +
						graph_array[number_of_graphs - 1].graph.x_number_of_major_grids + 1 +
						1 + 1;
g_print ("\tnumber_of_strings: %i\n", number_of_strings);

number_of_chars += total_len;
g_print ("\tnumber_of_chars: %i\n", number_of_chars);

//!!!Проверяем содержимое массива:
g_print ("СОДЕРЖИМОЕ МАССИВА:\n");
for (int i = 0; i < number_of_chars; i++)
{
	g_print ("\tЭлемент №%i; ", i);
	if (char_array[i] == '\0')
	{
		g_print ("Символ конца строки");
	} else {
		g_print ("Символ: %c", char_array[i]);
	}
	g_print ("; ASCII №%i;\n", char_array[i]);
}	//ВСЁ ВЕРНО

//Обновляю все графики и шрифты, так как адреса теперь новые
//Шрифты:
graph_appearance.tick_labels_font.name = (char *) ADN_font1;
graph_appearance.axis_labels_font.name = (char *) ADN_font2;
//К этому моменту количество графиков уже обновлено
unsigned int ADR2;
ADR2 = ADN_font2 + LEN_font2t * sizeof (char);	//Адрес y_ticks нулевого графика
for (unsigned int graph_number = 0; graph_number <= number_of_graphs - 1; graph_number++)
{
//Этот цикл не проверялся
	unsigned int LEN = 0;

	graph_array[graph_number].graph.y_tick_labels = (void *) ADR2;
	for (unsigned int i = 1; i <= graph_array[graph_number].graph.y_number_of_major_grids + 1; i++)
	{
		LEN = strlen ((char *) ADR2) + 1;
		ADR2 += LEN * sizeof (char);
	}
	graph_array[graph_number].graph.x_tick_labels = (void *) ADR2;
	for (unsigned int i = 1; i <= graph_array[graph_number].graph.x_number_of_major_grids + 1; i++)
	{
		LEN = strlen ((char *) ADR2) + 1;
		ADR2 += LEN * sizeof (char);
	}
	graph_array[graph_number].graph.y_axis_label = (void *) ADR2;
		LEN = strlen ((char *) ADR2) + 1;	
		ADR2 += LEN * sizeof (char);
	graph_array[graph_number].graph.x_axis_label = (void *) ADR2;
		LEN = strlen ((char *) ADR2) + 1;	
		ADR2 += LEN * sizeof (char);
}
//Обновление графиков завершено
}







static void
string_memory_allocate (enum op_type type_of_op)
{
	//В memory_free нужно будет чистить этот массив
}

//Функция изменения настроек внешнего вида графиков:
void
graph_set_appearance (struct graph_appearance new_graph_appearance)
{
	graph_appearance = new_graph_appearance;
//!!!Выделение памяти под новые строки:
	string_memory_allocate_FONT ();	//!!!
}

//Выделение памяти под графики:
static void
graph_memory_allocate (GtkDrawingArea *area, struct graph new_graph)
{
	g_print ("Начало выделения памяти\n");
	//Проверка принятой области рисования на совпадение с имеющимися в массиве графиков:
	bool area_match = FALSE;
	unsigned short int match_number;
	for (unsigned short int i = 1; i <= number_of_graphs; i++)
	{
		g_print ("Проверка на соответствие: итерация %i из %i\n", i, number_of_graphs);
		g_print ("Проверяется элемент массива %i\n", i - 1);
		g_print ("Сравнение зон по адресам: %p и %p\n", area, graph_array[i - 1].area);
		if (area == graph_array[i - 1].area)
		{
			area_match = TRUE;
			match_number = i - 1;
			g_print ("Совпадение с элементом: %i\n", match_number);
		}
		g_print ("Конец проверки на соответствие: area_match = %i\n", area_match);
	}

	//Обновление существующего графика:
	if (area_match)
	{
		g_print ("Обновление графика под номером %i\n", match_number);
		graph_array[match_number].graph = new_graph;
		//Обновление изображения:
		gtk_widget_hide (GTK_WIDGET (graph_array[match_number].area));
		gtk_widget_show (GTK_WIDGET (graph_array[match_number].area));

	//Создание нового графика:
	} else {
		g_print ("Создание нового графика\n");
		unsigned short int number_of_graphs_old;
		number_of_graphs_old = number_of_graphs;
		number_of_graphs++;
		//Отключение старых сигналов:
		if (number_of_graphs > 1)
		{
			for (unsigned short int graph_number = 0; graph_number <= number_of_graphs_old - 1; graph_number++)
			{
				g_signal_handlers_disconnect_by_func (graph_array[graph_number].area, (GFunc) configure_event_cb, &graph_array[graph_number].number);
				g_signal_handlers_disconnect_by_func (graph_array[graph_number].area, (GFunc) draw_cb, &graph_array[graph_number].number);
				g_signal_handlers_disconnect_by_func (graph_array[graph_number].area, (GFunc) size_allocate_cb, &graph_array[graph_number].number);
			}
		}
		//Выделение нового количества памяти под массив графиков:
		g_print ("Выделение памяти под %i элементов\n", number_of_graphs);
		graph_array = (struct graph_internal *) realloc (graph_array, number_of_graphs * sizeof (struct graph_internal));
		g_print ("Выделено %i байт\n", number_of_graphs * sizeof (struct graph_internal));
		//Добавление новых элементов в массив графиков:
		graph_array[number_of_graphs - 1].number = number_of_graphs - 1;
		graph_array[number_of_graphs - 1].area = area;
		GtkRequisition minimum_size;
		gtk_widget_get_preferred_size (GTK_WIDGET (area), &minimum_size, NULL);
		g_print ("minimum W:%d H:%d\n", minimum_size.width, minimum_size.height);
		graph_array[number_of_graphs - 1].area_height_request = minimum_size.height;
		graph_array[number_of_graphs - 1].area_width_request = minimum_size.width;
		cairo_surface_t *surface = NULL;
		graph_array[number_of_graphs - 1].surface = surface;
		cairo_t *context = NULL;
		graph_array[number_of_graphs - 1].context = context;
		graph_array[number_of_graphs - 1].graph = new_graph;

//!!!Выделение памяти под новые строки:
string_memory_allocate_GRAPH_ADD ();

		//Подключение новых сигналов:
		for (unsigned short int graph_number = 0; graph_number <= number_of_graphs - 1; graph_number++)
		{
			g_signal_connect (graph_array[graph_number].area, "configure-event", G_CALLBACK (configure_event_cb), &graph_array[graph_number].number);
			g_signal_connect (graph_array[graph_number].area, "draw", G_CALLBACK (draw_cb), &graph_array[graph_number].number);
			g_signal_connect (graph_array[graph_number].area, "size-allocate", G_CALLBACK (size_allocate_cb), &graph_array[graph_number].number);
		}
		//Обновление изображения:
		g_print ("Обновление картинки для нового графика:\n");
		gtk_widget_hide (GTK_WIDGET (graph_array[number_of_graphs - 1].area));
		gtk_widget_show (GTK_WIDGET (graph_array[number_of_graphs - 1].area));
		g_print ("Конец обновления картинки для нового графика:\n");
	}
	g_print ("Конец выделения памяти\n\n");
}

//Освобождение памяти, выделенной под графики:
void
graph_memory_free ()
{
	for (unsigned short int graph_number = 0; graph_number <= number_of_graphs - 1; graph_number++)
	{
		//Отключение сигналов:
		g_signal_handlers_disconnect_by_func (graph_array[graph_number].area, (GFunc) configure_event_cb, &graph_array[graph_number].number);
		g_signal_handlers_disconnect_by_func (graph_array[graph_number].area, (GFunc) draw_cb, &graph_array[graph_number].number);
		g_signal_handlers_disconnect_by_func (graph_array[graph_number].area, (GFunc) size_allocate_cb, &graph_array[graph_number].number);
		//Уничтожение поверхности Cairo:
		if (graph_array[graph_number].surface)
			cairo_surface_destroy (graph_array[graph_number].surface);
	}
	//Освобождение памяти, выделенной под массив:
	free (graph_array);
	//Обнуление количества графиков:
	number_of_graphs = 0;
}

//!!!Нужно придумать нормальное имя:
void
graph (GtkDrawingArea *area, struct graph new_graph)
{
	graph_memory_allocate (area, new_graph);
	//!!!Здесь будет запист ДАННЫХ графика в глобальную структуру
	//free(char_array);	//ошибка double free or corruption
}

//Очистка области рисования:
static void graph_clear (unsigned short int graph_number)
{
	graph_array[graph_number].context = cairo_create (graph_array[graph_number].surface);
	cairo_set_source_rgb (	graph_array[graph_number].context,
							graph_appearance.background_color.r,
							graph_appearance.background_color.g,
							graph_appearance.background_color.b);
	cairo_paint (graph_array[graph_number].context);
	cairo_destroy (graph_array[graph_number].context);
	//!!!Здесь возможно стоит прописать нулевой сайз реквест
	//!!!Возможно стоит эту функцию вывести в интерфейс
}



//Функция перевода размера шрифта из типографских пунктов в пиксели:
static guint
px (double pt)
{
	const double k = 0.752812499999996;
	return k * pt;
}

//Функция поиска максимального значения среди элементов типа double:
static guint
max (unsigned int num, ...)
{
	double cur = 0;
	double max = 0;
	va_list ap;
	va_start (ap, num);
	while (num--)
	{
		cur = va_arg (ap, double);
		max = (cur > max) ? cur : max;
	}
	va_end (ap);
	//Возвращаемое значение округляется вверх и приводится к типу guint:
	return (guint) ceil (max);
}

//Функция вычерчивания промежуточных линий сетки:
static void
minor_gridlines_plot (unsigned short int graph_number, struct geometry coordinate_plane_geometry, bool y_minor_gridlines, bool x_minor_gridlines)
{
	//Начальные установки:
	cairo_set_line_cap (graph_array[graph_number].context, CAIRO_LINE_CAP_BUTT);
	cairo_set_line_width (graph_array[graph_number].context, graph_appearance.minor_gridlines_thickness);
	cairo_set_source_rgb (graph_array[graph_number].context, graph_appearance.minor_gridlines_color.r, graph_appearance.minor_gridlines_color.g, graph_appearance.minor_gridlines_color.b);

	//Вычерчивание вертикальных промежуточных линий сетки:
	if (x_minor_gridlines)
	{
	double x_major_step = coordinate_plane_geometry.width / ceil (graph_array[graph_number].graph.x_number_of_major_grids);
		for (unsigned int i_major = 0; i_major < graph_array[graph_number].graph.x_number_of_major_grids; i_major++)
		{
			switch (graph_array[graph_number].graph.x_scale)
			{
				case LIN:
					for (unsigned int i_minor = 1; i_minor < graph_array[graph_number].graph.x_number_of_minor_grids; i_minor++)
					{
						cairo_move_to (graph_array[graph_number].context,
							coordinate_plane_geometry.x
								+ i_major * x_major_step
								+ i_minor * ((coordinate_plane_geometry.width / ceil ((graph_array[graph_number].graph.x_number_of_major_grids * graph_array[graph_number].graph.x_number_of_minor_grids)))),
							coordinate_plane_geometry.y);
						cairo_line_to (graph_array[graph_number].context,
							coordinate_plane_geometry.x
								+ i_major * x_major_step
								+ i_minor * ((coordinate_plane_geometry.width / ceil ((graph_array[graph_number].graph.x_number_of_major_grids * graph_array[graph_number].graph.x_number_of_minor_grids)))),
							coordinate_plane_geometry.y - coordinate_plane_geometry.height);
					}
					break;
				case LOG:
					for (unsigned int i_minor = 20; i_minor <= 90; i_minor += 10)
					{
						cairo_move_to (graph_array[graph_number].context,
							coordinate_plane_geometry.x
								+ i_major * x_major_step
								+ (log10 (i_minor) - 1.) * x_major_step,
							coordinate_plane_geometry.y);
						cairo_line_to (graph_array[graph_number].context,
							coordinate_plane_geometry.x
								+ i_major * x_major_step
								+ (log10 (i_minor) - 1.) * x_major_step,
							coordinate_plane_geometry.y - coordinate_plane_geometry.height);
					}
					break;
			}
		}
	}

	//Вычерчивание горизонтальных промежуточных линий сетки:
	if (y_minor_gridlines)
	{
	double y_major_step = coordinate_plane_geometry.height / ceil (graph_array[graph_number].graph.y_number_of_major_grids);
		for (unsigned int i_major = 0; i_major < graph_array[graph_number].graph.y_number_of_major_grids; i_major++)
		{
			switch (graph_array[graph_number].graph.y_scale)
			{
				case LIN:
					for (unsigned int i_minor = 1; i_minor < graph_array[graph_number].graph.y_number_of_minor_grids; i_minor++)
					{
						cairo_move_to (graph_array[graph_number].context,
							coordinate_plane_geometry.x,
							coordinate_plane_geometry.y
								- i_major * y_major_step
								- i_minor * ((coordinate_plane_geometry.height / ceil ((graph_array[graph_number].graph.y_number_of_major_grids * graph_array[graph_number].graph.y_number_of_minor_grids)))));
						cairo_line_to (graph_array[graph_number].context,
							coordinate_plane_geometry.x + coordinate_plane_geometry.width,
							coordinate_plane_geometry.y
								- i_major * y_major_step
								- i_minor * ((coordinate_plane_geometry.height / ceil ((graph_array[graph_number].graph.y_number_of_major_grids * graph_array[graph_number].graph.y_number_of_minor_grids)))));
					}
					break;
				case LOG:
					for (unsigned int i_minor = 20; i_minor <= 90; i_minor += 10)
					{
						cairo_move_to (graph_array[graph_number].context,
							coordinate_plane_geometry.x,
							coordinate_plane_geometry.y
								- i_major * y_major_step
								- (log10 (i_minor) - 1.) * y_major_step);
						cairo_line_to (graph_array[graph_number].context,
							coordinate_plane_geometry.x + coordinate_plane_geometry.width,
							coordinate_plane_geometry.y
								- i_major * y_major_step
								- (log10 (i_minor) - 1.) * y_major_step);
					}
					break;
			}
		}
	}

	cairo_stroke (graph_array[graph_number].context);
}

//Функция вычерчивания основных линий сетки:
static void
major_gridlines_plot (unsigned short int graph_number, struct geometry coordinate_plane_geometry)
{
	//Начальные установки:
	cairo_set_line_cap (graph_array[graph_number].context, CAIRO_LINE_CAP_SQUARE);
	cairo_set_line_width (graph_array[graph_number].context, graph_appearance.major_gridlines_thickness);
	cairo_set_source_rgb (graph_array[graph_number].context, graph_appearance.major_gridlines_color.r, graph_appearance.major_gridlines_color.g, graph_appearance.major_gridlines_color.b);

	//Вычерчивание вертикальных основных линий сетки:
	double x_major_step = coordinate_plane_geometry.width / ceil (graph_array[graph_number].graph.x_number_of_major_grids);
	for (unsigned int i = 0; i <= graph_array[graph_number].graph.x_number_of_major_grids; i++)
	{
		cairo_move_to (graph_array[graph_number].context, coordinate_plane_geometry.x + i * x_major_step, coordinate_plane_geometry.y);
		cairo_line_to (graph_array[graph_number].context, coordinate_plane_geometry.x + i * x_major_step, coordinate_plane_geometry.y - coordinate_plane_geometry.height);
	}

	//Вычерчивание горизонтальных основных линий сетки:
	double y_major_step = coordinate_plane_geometry.height / ceil (graph_array[graph_number].graph.y_number_of_major_grids);
	for (unsigned int i = 0; i <= graph_array[graph_number].graph.y_number_of_major_grids; i++)
	{
		cairo_move_to (graph_array[graph_number].context, coordinate_plane_geometry.x, coordinate_plane_geometry.y - i * y_major_step);
		cairo_line_to (graph_array[graph_number].context, coordinate_plane_geometry.x + coordinate_plane_geometry.width, coordinate_plane_geometry.y - i * y_major_step);
	}

	cairo_stroke (graph_array[graph_number].context);
}

//Функция вычерчивания меток осей:
static void
tick_labels_plot (unsigned short int graph_number, struct geometry *y_tick_labels_geometry, struct geometry *x_tick_labels_geometry)
{
	//Начальные установки:
	cairo_text_extents_t text_extents;
	char *text;
	cairo_set_source_rgb (graph_array[graph_number].context, graph_appearance.tick_labels_color.r, graph_appearance.tick_labels_color.g, graph_appearance.tick_labels_color.b);
	cairo_select_font_face (graph_array[graph_number].context, graph_appearance.tick_labels_font.name, graph_appearance.tick_labels_font.slant, graph_appearance.tick_labels_font.weight);
	cairo_set_font_size (graph_array[graph_number].context, graph_appearance.tick_labels_font.size);

	//Вычерчивание меток оси Y:
	text = (char *) graph_array[graph_number].graph.y_tick_labels;
	for (unsigned int i = 0; i <= graph_array[graph_number].graph.y_number_of_major_grids; i++)
	{
		cairo_move_to (graph_array[graph_number].context, y_tick_labels_geometry[i].x, y_tick_labels_geometry[i].y);
		cairo_text_extents (graph_array[graph_number].context, text, &text_extents);
		cairo_show_text (graph_array[graph_number].context, text);
		text += strlen (text) + 1;
	}

	//Вычерчивание меток оси X:
	text = (char *) graph_array[graph_number].graph.x_tick_labels;
	for (unsigned int i = 0; i <= graph_array[graph_number].graph.x_number_of_major_grids; i++)
	{
		cairo_move_to (graph_array[graph_number].context, x_tick_labels_geometry[i].x, x_tick_labels_geometry[i].y);
		cairo_text_extents (graph_array[graph_number].context, text, &text_extents);
		cairo_show_text (graph_array[graph_number].context, text);
		text += strlen (text) + 1;
	}

	cairo_fill (graph_array[graph_number].context);
}

//Функция вычерчивания подписей осей:
static void
axis_labels_plot (unsigned short int graph_number, struct geometry y_axis_label_geometry, struct geometry x_axis_label_geometry)
{
	//Начальные установки:
	cairo_text_extents_t text_extents;
	cairo_set_source_rgb (graph_array[graph_number].context, graph_appearance.axis_labels_color.r, graph_appearance.axis_labels_color.g, graph_appearance.axis_labels_color.b);
	cairo_select_font_face (graph_array[graph_number].context, graph_appearance.axis_labels_font.name, graph_appearance.axis_labels_font.slant, graph_appearance.axis_labels_font.weight);
	cairo_set_font_size (graph_array[graph_number].context, graph_appearance.axis_labels_font.size);

	//Вычерчивание подписи оси Y:
	cairo_move_to (graph_array[graph_number].context, y_axis_label_geometry.x, y_axis_label_geometry.y);
	cairo_text_extents (graph_array[graph_number].context, graph_array[graph_number].graph.y_axis_label, &text_extents);
	cairo_show_text (graph_array[graph_number].context, graph_array[graph_number].graph.y_axis_label);

	//Вычерчивание подписи оси X:
	cairo_move_to (graph_array[graph_number].context, x_axis_label_geometry.x, x_axis_label_geometry.y);
	cairo_text_extents (graph_array[graph_number].context, graph_array[graph_number].graph.x_axis_label, &text_extents);
	cairo_show_text (graph_array[graph_number].context, graph_array[graph_number].graph.x_axis_label);

	cairo_fill (graph_array[graph_number].context);
}

//Функция вычерчивания графика:
static void
graph_plot (unsigned short int graph_number, guint area_height, guint area_width)
{
	//Очистка области рисования:
	graph_clear (graph_number);

	graph_array[graph_number].context = cairo_create (graph_array[graph_number].surface);

	cairo_text_extents_t text_extents;

	//Размеры меток осей:
	cairo_select_font_face (graph_array[graph_number].context, graph_appearance.tick_labels_font.name, graph_appearance.tick_labels_font.slant, graph_appearance.tick_labels_font.weight);
	cairo_set_font_size (graph_array[graph_number].context, graph_appearance.tick_labels_font.size);
	//Размеры меток оси Y:
	struct geometry y_tick_labels_geometry[graph_array[graph_number].graph.y_number_of_major_grids + 1 - 1];
	char *text = (char *) graph_array[graph_number].graph.y_tick_labels;
	guint y_tick_labels_width_max = 0;
	for (unsigned int i = 0; i <= graph_array[graph_number].graph.y_number_of_major_grids + 1 - 1; i++)
	{
		cairo_text_extents (graph_array[graph_number].context, text, &text_extents);
		y_tick_labels_geometry[i].height = px (graph_appearance.tick_labels_font.size);
		y_tick_labels_geometry[i].width = text_extents.width;
		text += strlen (text) + 1;
		//Определение ширины наиболее широкой метки (за исключением верхней)
		if (i != graph_array[graph_number].graph.y_number_of_major_grids + 1 - 1)
			y_tick_labels_width_max = max (2, ceil (y_tick_labels_width_max), ceil (y_tick_labels_geometry[i].width));
	}
	//Размеры меток оси X:
	struct geometry x_tick_labels_geometry[graph_array[graph_number].graph.x_number_of_major_grids + 1 - 1];
	text = (char *) graph_array[graph_number].graph.x_tick_labels;
	guint x_tick_labels_width_average_max = 0;
	for (unsigned int i = 0; i <= graph_array[graph_number].graph.x_number_of_major_grids + 1 - 1; i++)
	{
		cairo_text_extents (graph_array[graph_number].context, text, &text_extents);
		x_tick_labels_geometry[i].height = px (graph_appearance.tick_labels_font.size);
		x_tick_labels_geometry[i].width = text_extents.width;		
		text += strlen (text) + 1;
		//Определение максимальной средней ширины двух соседних меток оси X:
		if (i != 0)
			x_tick_labels_width_average_max
				= max (2,
					ceil (x_tick_labels_width_average_max),
					ceil ((x_tick_labels_geometry[i].width + x_tick_labels_geometry[i - 1].width) / 2));
	}

	//Размеры подписей осей:
	cairo_select_font_face (graph_array[graph_number].context, graph_appearance.axis_labels_font.name, graph_appearance.axis_labels_font.slant, graph_appearance.axis_labels_font.weight);
	cairo_set_font_size (graph_array[graph_number].context, graph_appearance.axis_labels_font.size);
	//Размеры подписи оси Y:
	cairo_text_extents (graph_array[graph_number].context, graph_array[graph_number].graph.y_axis_label, &text_extents);
	struct geometry y_axis_label_geometry;
	y_axis_label_geometry.height = px (graph_appearance.axis_labels_font.size);
	y_axis_label_geometry.width = text_extents.width;
	//Размеры подписи оси X:
	cairo_text_extents (graph_array[graph_number].context, graph_array[graph_number].graph.x_axis_label, &text_extents);
	struct geometry x_axis_label_geometry;
	x_axis_label_geometry.height = px (graph_appearance.axis_labels_font.size);
	x_axis_label_geometry.width = text_extents.width;

	//Минимально допустимое количество пикселей между соседними элементами:
	guint sep = 2;

	//Вычисление требований к минимальным размерам области рисования:
	gint area_height_request;
	gint area_width_request;
	guint top_elements_height_request;
	guint coordinate_plane_height_request;
	guint bottom_elements_height_request;
	guint right_elements_width_request;
	guint coordinate_plane_width_request;
	guint left_elements_width_request;
	//Вычисление минимального размера виджета по вертикали:
	top_elements_height_request
		= graph_appearance.distance_widget_borders_to_axis_labels
		+ max (3,
			ceil (y_axis_label_geometry.height),
			ceil (y_tick_labels_geometry[graph_array[graph_number].graph.y_number_of_major_grids + 1 - 1].height),
			ceil (graph_appearance.major_gridlines_thickness))
		/ 2.;
	coordinate_plane_height_request
		= graph_array[graph_number].graph.y_number_of_major_grids
		* (max (2,
			ceil (y_tick_labels_geometry[0].height),
			ceil (graph_appearance.major_gridlines_thickness))
		+ sep);
	bottom_elements_height_request
		= graph_appearance.major_gridlines_thickness / 2.
		+ graph_appearance.distance_tick_labels_to_gridlines
		+ x_tick_labels_geometry[0].height / 2.
		+ max (2,
			ceil (x_tick_labels_geometry[0].height),
			ceil (x_axis_label_geometry.height))
		/ 2.
		+ graph_appearance.distance_widget_borders_to_axis_labels;
	area_height_request
		= top_elements_height_request
		+ coordinate_plane_height_request
		+ bottom_elements_height_request;
	//Вычисление минимального размера виджета по горизонтали:
	right_elements_width_request
		= graph_appearance.distance_widget_borders_to_axis_labels
		+ ((x_axis_label_geometry.height > x_tick_labels_geometry[0].height)
		? x_axis_label_geometry.width
			+ graph_appearance.distance_axis_labels_to_tick_labels
			+ max (2,
				ceil (graph_appearance.major_gridlines_thickness),
				ceil (x_tick_labels_geometry[graph_array[graph_number].graph.x_number_of_major_grids + 1 - 1].width))
			/ 2.
		: max (2,
			ceil (graph_appearance.major_gridlines_thickness / 2.),
			ceil (x_axis_label_geometry.width
				+ graph_appearance.distance_axis_labels_to_tick_labels
				+ x_tick_labels_geometry[graph_array[graph_number].graph.x_number_of_major_grids + 1 - 1].width / 2.)));
	coordinate_plane_width_request
		= graph_array[graph_number].graph.x_number_of_major_grids
		* (max (2,
			ceil (x_tick_labels_width_average_max),
			ceil (graph_appearance.major_gridlines_thickness))
		+ sep);
	left_elements_width_request
		= max (3,
			ceil (graph_appearance.major_gridlines_thickness / 2.
				+ graph_appearance.distance_tick_labels_to_gridlines
				+ y_tick_labels_geometry[graph_array[graph_number].graph.y_number_of_major_grids + 1 - 1].width
				+ graph_appearance.distance_axis_labels_to_tick_labels
				+ y_axis_label_geometry.width),
			ceil (graph_appearance.major_gridlines_thickness / 2.
				+ graph_appearance.distance_tick_labels_to_gridlines
				+ y_tick_labels_width_max),
			ceil (x_tick_labels_geometry[0].width / 2.))
		+ graph_appearance.distance_widget_borders_to_axis_labels;
	area_width_request
		= right_elements_width_request
		+ coordinate_plane_width_request
		+ left_elements_width_request;

	//Обновление минимальных размеров области рисования:
	if (area_height_request < graph_array[graph_number].area_height_request)
		area_height_request = graph_array[graph_number].area_height_request;
	if (area_width_request < graph_array[graph_number].area_width_request)
		area_width_request = graph_array[graph_number].area_width_request;
	gtk_widget_set_size_request (GTK_WIDGET (graph_array[graph_number].area), area_width_request, area_height_request);

	//Вычисление минимальных размеров области рисования, при которых будут вычерчиваться промежуточные линии сетки:
	gint minor_gridlines_height_request = -1;
	gint minor_gridlines_width_request = -1;
	//Вычисление размера по вертикали:
	if (graph_array[graph_number].graph.y_number_of_minor_grids > 1)
	{
		minor_gridlines_height_request = top_elements_height_request + bottom_elements_height_request;
		switch (graph_array[graph_number].graph.y_scale)
		{
			case LIN:
				minor_gridlines_height_request
					+= graph_array[graph_number].graph.y_number_of_major_grids
					* (graph_appearance.major_gridlines_thickness
						+ (graph_array[graph_number].graph.y_number_of_minor_grids - 1) * graph_appearance.minor_gridlines_thickness
						+ graph_array[graph_number].graph.y_number_of_minor_grids * sep);
				break;
			case LOG:
				if (((graph_appearance.major_gridlines_thickness + graph_appearance.major_gridlines_thickness) / 2. + sep) > (graph_appearance.minor_gridlines_thickness + sep))
				{
					minor_gridlines_height_request
						+= graph_array[graph_number].graph.y_number_of_major_grids
						* 1. / (2. - log10 (90.))
						* ((graph_appearance.major_gridlines_thickness + graph_appearance.minor_gridlines_thickness) / 2. + sep);
				} else {
					minor_gridlines_height_request
						+= graph_array[graph_number].graph.y_number_of_major_grids
						* 1. / (log10 (90.) - log10 (80.))
						* (graph_appearance.minor_gridlines_thickness + sep);
				}
				break;
		}
	}
	//Вычисление размера по горизонтали:
	if (graph_array[graph_number].graph.x_number_of_minor_grids > 1)
	{
		minor_gridlines_width_request = right_elements_width_request + left_elements_width_request;
		switch (graph_array[graph_number].graph.x_scale)
		{
			case LIN:
				minor_gridlines_width_request
					+= graph_array[graph_number].graph.x_number_of_major_grids
					* (graph_appearance.major_gridlines_thickness
						+ (graph_array[graph_number].graph.x_number_of_minor_grids - 1) * graph_appearance.minor_gridlines_thickness
						+ graph_array[graph_number].graph.x_number_of_minor_grids * sep);
				break;
			case LOG:
				if (((graph_appearance.major_gridlines_thickness + graph_appearance.major_gridlines_thickness) / 2. + sep) > (graph_appearance.minor_gridlines_thickness + sep))
				{
					minor_gridlines_width_request
						+= graph_array[graph_number].graph.x_number_of_major_grids
						* 1. / (2. - log10 (90.))
						* ((graph_appearance.major_gridlines_thickness + graph_appearance.minor_gridlines_thickness) / 2. + sep);
				} else {
					minor_gridlines_width_request
						+= graph_array[graph_number].graph.x_number_of_major_grids
						* 1. / (log10 (90.) - log10 (80.))
						* (graph_appearance.minor_gridlines_thickness + sep);
				}
				break;
		}
	}

	//Принятие решения о возможности построения вспомогательных линий сетки:
	bool y_minor_gridlines = FALSE;
	bool x_minor_gridlines = FALSE;
	if ((area_height >= minor_gridlines_height_request) && (minor_gridlines_height_request != -1))
		y_minor_gridlines = TRUE;
	if ((area_width >= minor_gridlines_width_request) && (minor_gridlines_width_request != -1))
		x_minor_gridlines = TRUE;

	//Геометрия координатной плоскости:
	//Толщина линий здесь не учитывается
	struct geometry coordinate_plane_geometry;
	//X и Y - это координаты левого нижнего угла координатной плоскости
	coordinate_plane_geometry.y	= area_height - bottom_elements_height_request;
	coordinate_plane_geometry.x = left_elements_width_request;
	coordinate_plane_geometry.height = area_height - (bottom_elements_height_request + top_elements_height_request);
	coordinate_plane_geometry.width = area_width - (left_elements_width_request + right_elements_width_request);

	//Координаты меток осей:
	//Координаты меток оси Y:
	for (unsigned int i = 0; i <= graph_array[graph_number].graph.y_number_of_major_grids + 1 - 1; i++)
	{
		y_tick_labels_geometry[i].y
			= coordinate_plane_geometry.y
			- i * (coordinate_plane_geometry.height / graph_array[graph_number].graph.y_number_of_major_grids)
			+ y_tick_labels_geometry[0].height / 2.;
		y_tick_labels_geometry[i].x
			= coordinate_plane_geometry.x
			- graph_appearance.major_gridlines_thickness / 2.
			- graph_appearance.distance_tick_labels_to_gridlines
			- y_tick_labels_geometry[i].width;
	}
	//Координаты меток оси X:
	for (unsigned int i = 0; i <= graph_array[graph_number].graph.x_number_of_major_grids + 1 - 1; i++)
	{
		x_tick_labels_geometry[i].y
			= coordinate_plane_geometry.y
			+ graph_appearance.major_gridlines_thickness / 2.
			+ graph_appearance.distance_tick_labels_to_gridlines
			+ x_tick_labels_geometry[0].height;
		x_tick_labels_geometry[i].x
			= coordinate_plane_geometry.x
			+ i * (coordinate_plane_geometry.width / graph_array[graph_number].graph.x_number_of_major_grids)
			- x_tick_labels_geometry[i].width / 2.;
	}

	//Координаты подписей осей:
	//Координаты подписи оси Y:
	y_axis_label_geometry.y
		= coordinate_plane_geometry.y
		- coordinate_plane_geometry.height
		+ y_axis_label_geometry.height / 2.;
	y_axis_label_geometry.x
		= coordinate_plane_geometry.x
		- graph_appearance.major_gridlines_thickness / 2.
		- graph_appearance.distance_tick_labels_to_gridlines
		- y_tick_labels_geometry[graph_array[graph_number].graph.y_number_of_major_grids + 1 - 1].width
		- graph_appearance.distance_axis_labels_to_tick_labels
		- y_axis_label_geometry.width;
	//Координаты подписи оси X:
	x_axis_label_geometry.y
		= coordinate_plane_geometry.y
		+ graph_appearance.major_gridlines_thickness / 2.
		+ graph_appearance.distance_tick_labels_to_gridlines
		+ x_tick_labels_geometry[0].height / 2.
		+ x_axis_label_geometry.height / 2.;
	x_axis_label_geometry.x
		= area_width
		- graph_appearance.distance_widget_borders_to_axis_labels
	- x_axis_label_geometry.width;

//!!!Построение графика:
minor_gridlines_plot (graph_number, coordinate_plane_geometry, y_minor_gridlines, x_minor_gridlines);
major_gridlines_plot (graph_number, coordinate_plane_geometry);
//traces_plot()
tick_labels_plot (graph_number, y_tick_labels_geometry, x_tick_labels_geometry);
axis_labels_plot (graph_number, y_axis_label_geometry, x_axis_label_geometry);

	cairo_destroy (graph_array[graph_number].context);

	gtk_widget_queue_draw_area (GTK_WIDGET (graph_array[graph_number].area), 0, 0, area_width, area_height);
}
