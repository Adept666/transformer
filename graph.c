//Модуль построения графиков
//v0.001.
#include <gtk/gtk.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

//Цвет:
struct color {
	double r;
	double g;
	double b;
};

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

//Текущие настройки внешнего вида графиков:
static struct graph_appearance graph_appearance;

//Шкала:
enum scale {
	//Линейная:
	LIN,
	//Логарифмическая:
	LOG
};

//Координаты точки трассы:
struct point {
	double x;
	double y;
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
	//Количество промежуточных сеток оси Y:
	unsigned int y_number_of_minor_grids;
	//Количество промежуточных сеток оси X:
	unsigned int x_number_of_minor_grids;
	//Метки оси Y:
	char *y_tick_labels;	//!!!
	//Метки оси X:
	char *x_tick_labels;	//!!!
	//Подпись оси Y:
	char *y_axis_label;
	//Подпись оси X:
	char *x_axis_label;
	//Минимальные и максимальные значения Y и X:
	double y_value_min;
	double y_value_max;
	double x_value_min;
	double x_value_max;
	//Количество точек трассы:
	unsigned int trace_number_of_points;
	//Точки трассы:
	struct point *trace_points;
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
static struct graph_internal *graph_array;
//Текущее количество запомненных графиков:
static unsigned short int number_of_graphs = 0;

//Динамический массив символов:
static char *char_array;
//Текущее количество запомненных символов:
static unsigned int number_of_chars = 0;

//Динамический массив точек:
static struct point *point_array;
//Текущее количество запомненных точек:
static unsigned int number_of_points = 0;

//Геометрия элемента графика:
struct geometry {
	guint y;
	guint x;
	guint height;
	guint width;
};

//!!!ГРАНИЦА ЧИСТОВОГО И ЧЕРНОВОГО КОДА:
//Работа с памятью завершена полностью
//НУЖНО СДЕЛАТЬ:
	//1) Переписать plot_trace так, чтобы функция поддерживала ЛОГ масштаб
	//1.1) Нужно разобраться с ошибкой сдвига символов. Для этого вывожу все символы в специальной функции
		//В char_array отсутствует символ пропуска строки. Нужно посмотреть алгаритм заполнения char_array при Add и Update (ошибки и там и там)
		//Аксис лейблы Ось Y и Ось X перепутаны
		//Вместо Xticks строятся Yticks
	//2) Найти ошибку в вычислении размеров, из-за которой tick_labels строятся не совсем корректно
	//3) Продумать композицию модуля

//Объявление функций:
static void clear_graph (unsigned short int graph_number);
static void plot_graph (unsigned short int graph_number, guint area_height, guint area_width);

static void
configure_event_cb (GtkWidget *drawing_area, GdkEventConfigure *event, gpointer data)
{
	unsigned short int n;
	memcpy(&n, data, sizeof (unsigned short int));
	if (graph_array[n].surface)
		cairo_surface_destroy (graph_array[n].surface);
	graph_array[n].surface = gdk_window_create_similar_surface (gtk_widget_get_window (GTK_WIDGET (graph_array[n].area)),
		CAIRO_CONTENT_COLOR,
		gtk_widget_get_allocated_width (GTK_WIDGET (graph_array[n].area)),
		gtk_widget_get_allocated_height (GTK_WIDGET (graph_array[n].area)));
	//Очистка поверхности:
	clear_graph (n);
}

static void
draw_cb (GtkWidget *drawing_area, cairo_t *context, gpointer data)
{
	unsigned short int n;
	memcpy(&n, data, sizeof (unsigned short int));
	cairo_set_source_surface (graph_array[n].context, graph_array[n].surface, 0, 0);
	cairo_paint (graph_array[n].context);
}

static void
size_allocate_cb (GtkWidget *drawing_area, GdkRectangle *allocation, gpointer data)
{
	unsigned short int n;
	memcpy(&n, data, sizeof (unsigned short int));
	guint area_height, area_width;
	area_height = gtk_widget_get_allocated_height (GTK_WIDGET (graph_array[n].area));
	area_width = gtk_widget_get_allocated_width (GTK_WIDGET (graph_array[n].area));
//	plot_graph (n, area_height, area_width);
}

//!!!Переделал цикл в конце. Сейчас должно быть норм, но проверять рано.
//Функция выделения памяти под строки при обновлении шрифтов:
static void
string_memory_allocate_fonts_update ()
{
	//Первичное выделение памяти под массив символов:
	if (number_of_chars == 0)
	{
		char_array = (char *) malloc (2 * sizeof (char));
		char_array[0] = '\0';
		char_array[1] = '\0';
		number_of_chars = 2;
	}

	//Старый адрес имени шрифта меток осей в массиве символов:
	unsigned int font_tl_addr_old;
	font_tl_addr_old = (unsigned int) char_array;
	//Старое кол-во символов в имени шрифта меток осей (с учётом \0):
	unsigned int font_tl_len_old;
	font_tl_len_old = strlen ((char *) font_tl_addr_old) + 1;
	//Старый адрес имени шрифта подписей осей в массиве символов:
	unsigned int font_al_addr_old;
	font_al_addr_old = font_tl_addr_old + font_tl_len_old * sizeof (char);
	//Старое кол-во символов в имени шрифта подписей осей (с учётом \0):
	unsigned int font_al_len_old;
	font_al_len_old = strlen ((char *) font_al_addr_old) + 1;
	//Общее количество символов в именах шрифтов (с учётом \0):
	unsigned int fonts_len_old;
	fonts_len_old = font_tl_len_old + font_al_len_old;

	//Создание массива временного хранения:
	char *bufarray_top;
	//Вычисление требуемого объёма памяти массива временного хранения:
	unsigned int bufarray_top_number_of_chars;
	bufarray_top_number_of_chars = number_of_chars - fonts_len_old;
	unsigned int bufarray_top_size;
	bufarray_top_size = bufarray_top_number_of_chars * sizeof (char);
	//Выделение памяти под массив временного хранения:
	if (bufarray_top_size)
	{
		bufarray_top = (char *) malloc (bufarray_top_size);
		//Копирование старых символов в массив временного хранения:
		memcpy (bufarray_top, (char *) (font_al_addr_old + font_al_len_old * sizeof (char)), bufarray_top_size);
	}

	//Принятые адреса имён шрифтов:
	unsigned int font_tl_addr_received;
	font_tl_addr_received = (unsigned int) graph_appearance.tick_labels_font.name;
	unsigned int font_al_addr_received;
	font_al_addr_received = (unsigned int) graph_appearance.axis_labels_font.name;
	//Кол-во символов в новых именах шрифтов (с учётом \0):
	unsigned int font_tl_len_new;
	font_tl_len_new = strlen ((char *) font_tl_addr_received) + 1;
	unsigned int font_al_len_new;
	font_al_len_new = strlen ((char *) font_al_addr_received) + 1;
	//Общее кол-во символов в новых именах шрифтов (с учётом \0):
	unsigned int fonts_len_new;
	fonts_len_new = font_tl_len_new + font_al_len_new;

	//Выделение нового количества памяти под массив символов:
	free (char_array);
	char_array = (char *) malloc ((fonts_len_new + bufarray_top_number_of_chars) * sizeof (char));

	//Новые адреса имён шрифтов в массиве символов:
	unsigned int font_tl_addr_new;
	font_tl_addr_new = (unsigned int) char_array;
	unsigned int font_al_addr_new;
	font_al_addr_new = font_tl_addr_new + font_tl_len_new * sizeof (char);

	//Восстановление старых символов из массива временного хранения:
	if (bufarray_top_size)
	{
		memcpy ((char *) (font_al_addr_new + font_al_len_new * sizeof (char)), bufarray_top, bufarray_top_size);
		//Освобождение памяти, выделенной под массив временного хранения:
		free (bufarray_top);
	}

	//Копирование новых символов в массив символов:
	memcpy ((char *) font_tl_addr_new, (char *) font_tl_addr_received, font_tl_len_new * sizeof (char));
	memcpy ((char *) font_al_addr_new, (char *) font_al_addr_received, font_al_len_new * sizeof (char));

	//Обновление адресов имён шрифтов:
	graph_appearance.tick_labels_font.name = (char *) font_tl_addr_new;
	graph_appearance.axis_labels_font.name = (char *) font_al_addr_new;

	//Обновление адресов меток и подписей осей:
	if (bufarray_top_size)
	{
		unsigned int addr_new;
		unsigned int len_new;
		addr_new = font_al_addr_new + font_al_len_new * sizeof (char);
		for (unsigned short int i = 1; i <= number_of_graphs; i++)
		{
			len_new = 0;
			//Обновление адресов меток оси Y:
			graph_array[i - 1].graph.y_tick_labels = (char *) addr_new;
			/*for (unsigned int j = 1; j <= graph_array[i - 1].graph.y_number_of_major_grids + 1; j++)
			{
				len_new = strlen ((char *) addr_new) + 1;	//!!!Теперь длина строки - это длина всех TL одного графика
				addr_new += len_new * sizeof (char);
			}*/
			len_new = strlen ((char *) addr_new) + 1;	//!!!Теперь длина строки - это длина всех TL одного графика
			addr_new += len_new * sizeof (char);
			//Обновление адресов меток оси X:
			graph_array[i - 1].graph.x_tick_labels = (char *) addr_new;
			/*for (unsigned int j = 1; j <= graph_array[i - 1].graph.x_number_of_major_grids + 1; j++)
			{
				len_new = strlen ((char *) addr_new) + 1;
				addr_new += len_new * sizeof (char);
			}*/
			len_new = strlen ((char *) addr_new) + 1;	//!!!Теперь длина строки - это длина всех TL одного графика
			addr_new += len_new * sizeof (char);
			//Обновление адреса подписи оси Y:
			graph_array[i - 1].graph.y_axis_label = (char *) addr_new;
			len_new = strlen ((char *) addr_new) + 1;	
			addr_new += len_new * sizeof (char);
			//Обновление адреса подписи оси X:
			graph_array[i - 1].graph.x_axis_label = (char *) addr_new;
			len_new = strlen ((char *) addr_new) + 1;	
			addr_new += len_new * sizeof (char);
		}
	}

	//Обновление количества символов:
	number_of_chars = bufarray_top_number_of_chars + fonts_len_new;
}

//!!!Всё переделал. Вроде норм но надо проверить
//Функция выделения памяти под строки при создании нового графика:
static void
string_memory_allocate_graph_add ()
{
g_print ("Начало str_malloc_graph_add\n");//!!!
	//Первичное выделение памяти под массив символов:
	if (number_of_chars == 0)
	{
		char_array = (char *) malloc (2 * sizeof (char));
		char_array[0] = '\0';
		char_array[1] = '\0';
		number_of_chars = 2;
	}

g_print ("\tСоздание bufarray_bottom\n");//!!!
	//Создание массива временного хранения:
	char *bufarray_bottom;
	//Выделение памяти под массив временного хранения:
	bufarray_bottom = (char *) malloc (number_of_chars * sizeof (char));
	//Копирование старых символов в массив временного хранения:
	memcpy (bufarray_bottom, char_array, number_of_chars * sizeof (char));

	//Принятые адреса меток и подписей осей:
	unsigned int y_tick_labels_addr_received;
	unsigned int x_tick_labels_addr_received;
	unsigned int y_axis_labels_addr_received;
	unsigned int x_axis_labels_addr_received;
	//Кол-во символов в принятых метках и подписях осей (с учётом \0):
	unsigned int y_tick_labels_len_received = 0;
	unsigned int x_tick_labels_len_received = 0;
	unsigned int y_axis_labels_len_received = 0;
	unsigned int x_axis_labels_len_received = 0;

g_print ("\tОпределение принятого адреса и количества символов\n");//!!!
	//Определение принятого адреса и нового кол-ва символов метки оси Y:
	unsigned int addr_received;
/*	for (unsigned int i = 0; i <= graph_array[number_of_graphs - 1].graph.y_number_of_major_grids; i++)
	{
		memcpy (&addr_received, graph_array[number_of_graphs - 1].graph.y_tick_labels + i * sizeof (unsigned int), sizeof (unsigned int));
		y_tick_labels_len_received += strlen ((char *) addr_received) + 1;
	}
	memcpy (&y_tick_labels_addr_received, graph_array[number_of_graphs - 1].graph.y_tick_labels, sizeof (unsigned int));
*/
y_tick_labels_addr_received = (unsigned int) graph_array[number_of_graphs - 1].graph.y_tick_labels;
y_tick_labels_len_received = strlen (graph_array[number_of_graphs - 1].graph.y_tick_labels) + 1;
g_print ("\tКол-во принятых символов Y_TICKS (с 0): %i\n", y_tick_labels_len_received);//!!!

	//Определение принятого адреса и нового кол-ва символов метки оси X:
/*	for (unsigned int i = 0; i <= graph_array[number_of_graphs - 1].graph.x_number_of_major_grids; i++)
	{
		memcpy (&addr_received, graph_array[number_of_graphs - 1].graph.x_tick_labels + i * sizeof (unsigned int), sizeof (unsigned int));
		x_tick_labels_len_received += strlen ((char *) addr_received) + 1;
	}
	memcpy (&x_tick_labels_addr_received, graph_array[number_of_graphs - 1].graph.x_tick_labels, sizeof (unsigned int));
*/
x_tick_labels_addr_received = (unsigned int) graph_array[number_of_graphs - 1].graph.x_tick_labels;
x_tick_labels_len_received = strlen (graph_array[number_of_graphs - 1].graph.x_tick_labels) + 1;
g_print ("\tКол-во принятых символов X_TICKS (с 0): %i\n", x_tick_labels_len_received);//!!!

	//Определение принятого адреса и нового кол-ва символов подписи оси Y:
	memcpy (&y_axis_labels_addr_received, &graph_array[number_of_graphs - 1].graph.y_axis_label, sizeof (unsigned int));
	y_axis_labels_len_received = strlen ((char *) y_axis_labels_addr_received) + 1;
g_print ("\tКол-во принятых символов Y_AXIS (с 0): %i\n", y_axis_labels_len_received);//!!!

	//Определение принятого адреса и нового кол-ва символов подписи оси X:
	memcpy (&x_axis_labels_addr_received, &graph_array[number_of_graphs - 1].graph.x_axis_label, sizeof (unsigned int));
	x_axis_labels_len_received = strlen ((char *) x_axis_labels_addr_received) + 1;
g_print ("\tКол-во принятых символов X_AXIS (с 0): %i\n", x_axis_labels_len_received);//!!!

	//Общее количество принятых символов (с учётом \0):
	unsigned int total_len_received;
	total_len_received = y_tick_labels_len_received + x_tick_labels_len_received + y_axis_labels_len_received + x_axis_labels_len_received;
g_print ("\tОбщее кол-во принятых символов (с нулями): %i\n", total_len_received);//!!!

	//Выделение нового количества памяти под массив символов:
	free (char_array);
	char_array = (char *) malloc ((number_of_chars + total_len_received) * sizeof (char));

	//Восстановление старых символов из массива временного хранения:
	memcpy (char_array, bufarray_bottom, number_of_chars * sizeof (char));
	//Освобождение памяти, выделенной под массив временного хранения:
	free (bufarray_bottom);

	//Адреса шрифтов и новых символов в массиве символов:
	unsigned int font_tl_addr_new;
	font_tl_addr_new = (unsigned int) char_array;
	unsigned int font_tl_len_new;
	font_tl_len_new = strlen ((char *) font_tl_addr_new) + 1;
	unsigned int font_al_addr_new;
	font_al_addr_new = font_tl_addr_new + font_tl_len_new * sizeof (char);
	unsigned int font_al_len_new;
	font_al_len_new = strlen ((char *) font_al_addr_new) + 1;
	unsigned int y_tick_labels_addr_new;
	y_tick_labels_addr_new = font_tl_addr_new + number_of_chars * sizeof (char);
	unsigned int x_tick_labels_addr_new;
	x_tick_labels_addr_new = y_tick_labels_addr_new + y_tick_labels_len_received * sizeof (char);
	unsigned int y_axis_labels_addr_new;									//!!!! НЕ лейблс а лейбл. и аналогично в других функциях
	y_axis_labels_addr_new = x_tick_labels_addr_new + x_tick_labels_len_received * sizeof (char);
	unsigned int x_axis_labels_addr_new;									//!!!! НЕ лейблс а лейбл. и аналогично в других функциях
	x_axis_labels_addr_new = y_axis_labels_addr_new + y_axis_labels_len_received * sizeof (char);

	//Копирование новых символов в массив символов:
g_print ("\ty_tick_labels_addr_received: %x\n", y_tick_labels_addr_received);//!!!
g_print ("\tx_tick_labels_addr_received: %x\n", x_tick_labels_addr_received);//!!!
g_print ("\ty_axis_labels_addr_received: %x\n", y_axis_labels_addr_received);//!!!
g_print ("\tx_axis_labels_addr_received: %x\n", x_axis_labels_addr_received);//!!!
	memcpy ((char *) y_tick_labels_addr_new, (char *) y_tick_labels_addr_received, y_tick_labels_len_received * sizeof (char));
	memcpy ((char *) x_tick_labels_addr_new, (char *) x_tick_labels_addr_received, x_tick_labels_len_received * sizeof (char));
	memcpy ((char *) y_axis_labels_addr_new, (char *) y_axis_labels_addr_received, y_axis_labels_len_received * sizeof (char));
	memcpy ((char *) x_axis_labels_addr_new, (char *) x_axis_labels_addr_received, x_axis_labels_len_received * sizeof (char));

	//Обновление адресов имён шрифтов:
	graph_appearance.tick_labels_font.name = (char *) font_tl_addr_new;
	graph_appearance.axis_labels_font.name = (char *) font_al_addr_new;

	//Обновление адресов меток и подписей осей:
	unsigned int addr_new;
	unsigned int len_new;
	addr_new = font_al_addr_new + font_al_len_new * sizeof (char);
	for (unsigned short int i = 1; i <= number_of_graphs; i++)
	{
		len_new = 0;
		//Обновление адресов меток оси Y:
		graph_array[i - 1].graph.y_tick_labels = (char *) addr_new;
		/*for (unsigned int j = 1; j <= graph_array[i - 1].graph.y_number_of_major_grids + 1; j++)
		{
			len_new = strlen ((char *) addr_new) + 1;
			addr_new += len_new * sizeof (char);
		}*/
		len_new = strlen ((char *) addr_new) + 1;	//!!!Теперь длина строки - это длина всех TL одного графика
		addr_new += len_new * sizeof (char);
		//Обновление адресов меток оси X:
		graph_array[i - 1].graph.x_tick_labels = (char *) addr_new;
		/*for (unsigned int j = 1; j <= graph_array[i - 1].graph.x_number_of_major_grids + 1; j++)
		{
			len_new = strlen ((char *) addr_new) + 1;
			addr_new += len_new * sizeof (char);
		}*/
		len_new = strlen ((char *) addr_new) + 1;	//!!!Теперь длина строки - это длина всех TL одного графика
		addr_new += len_new * sizeof (char);
		//Обновление адреса подписи оси Y:
		graph_array[i - 1].graph.y_axis_label = (void *) addr_new;
		len_new = strlen ((char *) addr_new) + 1;	
		addr_new += len_new * sizeof (char);
		//Обновление адреса подписи оси X:
		graph_array[i - 1].graph.x_axis_label = (void *) addr_new;
		len_new = strlen ((char *) addr_new) + 1;	
		addr_new += len_new * sizeof (char);
	}

	//Обновление количества символов:
	number_of_chars += total_len_received;
g_print ("Конец str_malloc_graph_add\n");//!!!
}
/*
//Функция выделения памяти под строки при обновлении существующего графика:
static void
string_memory_allocate_graph_update (unsigned short int graph_number)
{
	//Создание нижнего буфера:
	char *bufarray_bottom;
	unsigned int bufarray_bottom_number_of_strings = 2;
	unsigned int bufarray_bottom_number_of_chars = 0;
	unsigned int bufarray_bottom_size;
	//Создание верхнего буфера:
	char *bufarray_top;
	unsigned int bufarray_top_number_of_strings = 0;
	unsigned int bufarray_top_number_of_chars = 0;
	unsigned int bufarray_top_size;

	//Подсчёт количества строк, сохраняемых в нижнем буфере:
	for (unsigned short int i = 1; i < graph_number + 1; i++)
	{
		bufarray_bottom_number_of_strings
			+= graph_array[i - 1].graph.y_number_of_major_grids + 1
			+ graph_array[i - 1].graph.x_number_of_major_grids + 1
			+ 2;
	}
	//Подсчёт количества символов, сохраняемых в нижнем буфере:
	unsigned int addr_old;
	addr_old = (unsigned int) char_array;
	unsigned int len_old;
	for (unsigned int i = 1; i <= bufarray_bottom_number_of_strings; i++)
	{
		len_old = strlen ((char *) addr_old) + 1;
		bufarray_bottom_number_of_chars += len_old;
		addr_old += len_old * sizeof (char);
	}
	//Вычисление требуемого объёма памяти нижнего буфера:
	bufarray_bottom_size = bufarray_bottom_number_of_chars * sizeof (char);

	//Подсчёт количества строк, сохраняемых в верхнем буфере:
	for (unsigned int i = graph_number + 1 + 1; i <= number_of_graphs; i++)
	{
		bufarray_top_number_of_strings
			+= graph_array[i - 1].graph.y_number_of_major_grids + 1
			+ graph_array[i - 1].graph.x_number_of_major_grids + 1
			+ 2;
	}
	//Подсчёт количества символов, сохраняемых в верхнем буфере:
	if (bufarray_top_number_of_strings)
		addr_old = (unsigned int) graph_array[graph_number + 1].graph.y_tick_labels;
	for (unsigned int i = 1; i <= bufarray_top_number_of_strings; i++)
	{
		len_old = strlen ((char *) addr_old) + 1;
		bufarray_top_number_of_chars += len_old;
		addr_old += len_old * sizeof (char);
	}
	//Вычисление требуемого объёма памяти нижнего буфера:
	bufarray_top_size = bufarray_top_number_of_chars * sizeof (char);

	//Выделение памяти под массивы временного хранения:
	bufarray_bottom = (char *) malloc (bufarray_bottom_size);
	bufarray_top = (char *) malloc (bufarray_top_size);
	//Копирование старых символов в массивы временного хранения:
	memcpy (bufarray_bottom, char_array, bufarray_bottom_size);
	memcpy (bufarray_top, graph_array[graph_number + 1].graph.y_tick_labels, bufarray_top_size);

	//Принятые адреса меток и подписей осей:
	unsigned int y_tick_labels_addr_received;
	unsigned int x_tick_labels_addr_received;
	unsigned int y_axis_labels_addr_received;
	unsigned int x_axis_labels_addr_received;
	//Кол-во символов в принятых метках и подписях осей (с учётом \0):
	unsigned int y_tick_labels_len_received = 0;
	unsigned int x_tick_labels_len_received = 0;
	unsigned int y_axis_labels_len_received = 0;
	unsigned int x_axis_labels_len_received = 0;

	//Определение принятого адреса и нового кол-ва символов метки оси Y:
	unsigned int addr_received;
	for (unsigned int i = 0; i <= graph_array[graph_number].graph.y_number_of_major_grids; i++)
	{
		memcpy (&addr_received, graph_array[graph_number].graph.y_tick_labels + i * sizeof (unsigned int), sizeof (unsigned int));
		y_tick_labels_len_received += strlen ((char *) addr_received) + 1;
	}
	memcpy (&y_tick_labels_addr_received, graph_array[graph_number].graph.y_tick_labels, sizeof (unsigned int));

	//Определение принятого адреса и нового кол-ва символов метки оси X:
	for (unsigned int i = 0; i <= graph_array[graph_number].graph.x_number_of_major_grids; i++)
	{
		memcpy (&addr_received, graph_array[graph_number].graph.x_tick_labels + i * sizeof (unsigned int), sizeof (unsigned int));
		x_tick_labels_len_received += strlen ((char *) addr_received) + 1;
	}
	memcpy (&x_tick_labels_addr_received, graph_array[graph_number].graph.x_tick_labels, sizeof (unsigned int));

	//Определение принятого адреса и нового кол-ва символов подписи оси Y:
	memcpy (&y_axis_labels_addr_received, &graph_array[graph_number].graph.y_axis_label, sizeof (unsigned int));
	y_axis_labels_len_received = strlen ((char *) y_axis_labels_addr_received) + 1;

	//Определение принятого адреса и нового кол-ва символов подписи оси X:
	memcpy (&x_axis_labels_addr_received, &graph_array[graph_number].graph.x_axis_label, sizeof (unsigned int));
	x_axis_labels_len_received = strlen ((char *) x_axis_labels_addr_received) + 1;

	//Общее количество принятых символов (с учётом \0):
	unsigned int total_len_received;
	total_len_received = y_tick_labels_len_received + x_tick_labels_len_received + y_axis_labels_len_received + x_axis_labels_len_received;

	//Выделение нового количества памяти под массив символов:
	free (char_array);
	char_array = (char *) malloc (bufarray_bottom_size + total_len_received * sizeof (char) + bufarray_top_size);

	//Восстановление старых символов из массивов временного хранения:
	memcpy (char_array, bufarray_bottom, bufarray_bottom_size);
	memcpy ((char *) ((unsigned int) char_array + bufarray_bottom_size + total_len_received * sizeof (char)),
		bufarray_top,
		bufarray_top_size);

	//Освобождение памяти, выделенной под массивы временного хранения:
	free (bufarray_bottom);
	free (bufarray_top);

	//Адреса шрифтов и новых символов в массиве символов:
	unsigned int font_tl_addr_new;
	font_tl_addr_new = (unsigned int) char_array;
	unsigned int font_tl_len_new;
	font_tl_len_new = strlen ((char *) font_tl_addr_new) + 1;
	unsigned int font_al_addr_new;
	font_al_addr_new = font_tl_addr_new + font_tl_len_new * sizeof (char);
	unsigned int font_al_len_new;
	font_al_len_new = strlen ((char *) font_al_addr_new) + 1;
	unsigned int y_tick_labels_addr_new;
	y_tick_labels_addr_new = font_tl_addr_new + bufarray_bottom_size;
	unsigned int x_tick_labels_addr_new;
	x_tick_labels_addr_new = y_tick_labels_addr_new + y_tick_labels_len_received * sizeof (char);
	unsigned int y_axis_labels_addr_new;
	y_axis_labels_addr_new = x_tick_labels_addr_new + x_tick_labels_len_received * sizeof (char);
	unsigned int x_axis_labels_addr_new;
	x_axis_labels_addr_new = y_axis_labels_addr_new + y_axis_labels_len_received * sizeof (char);

	//Копирование новых символов в массив символов:
	memcpy ((char *) y_tick_labels_addr_new, (char *) y_tick_labels_addr_received, y_tick_labels_len_received * sizeof (char));
	memcpy ((char *) x_tick_labels_addr_new, (char *) x_tick_labels_addr_received, x_tick_labels_len_received * sizeof (char));
	memcpy ((char *) y_axis_labels_addr_new, (char *) y_axis_labels_addr_received, y_axis_labels_len_received * sizeof (char));
	memcpy ((char *) x_axis_labels_addr_new, (char *) x_axis_labels_addr_received, x_axis_labels_len_received * sizeof (char));

	//Обновление адресов имён шрифтов:
	graph_appearance.tick_labels_font.name = (char *) font_tl_addr_new;
	graph_appearance.axis_labels_font.name = (char *) font_al_addr_new;

	//Обновление адресов меток и подписей осей:
	unsigned int addr_new;
	unsigned int len_new;
	addr_new = font_al_addr_new + font_al_len_new * sizeof (char);
	for (unsigned short int i = 1; i <= number_of_graphs; i++)
	{
		len_new = 0;
		//Обновление адресов меток оси Y:
		graph_array[i - 1].graph.y_tick_labels = (void *) addr_new;
		for (unsigned int j = 1; j <= graph_array[i - 1].graph.y_number_of_major_grids + 1; j++)
		{
			len_new = strlen ((char *) addr_new) + 1;
			addr_new += len_new * sizeof (char);
		}
		//Обновление адресов меток оси X:
		graph_array[i - 1].graph.x_tick_labels = (void *) addr_new;
		for (unsigned int j = 1; j <= graph_array[i - 1].graph.x_number_of_major_grids + 1; j++)
		{
			len_new = strlen ((char *) addr_new) + 1;
			addr_new += len_new * sizeof (char);
		}
		//Обновление адреса подписи оси Y:
		graph_array[i - 1].graph.y_axis_label = (void *) addr_new;
		len_new = strlen ((char *) addr_new) + 1;	
		addr_new += len_new * sizeof (char);
		//Обновление адреса подписи оси X:
		graph_array[i - 1].graph.x_axis_label = (void *) addr_new;
		len_new = strlen ((char *) addr_new) + 1;	
		addr_new += len_new * sizeof (char);
	}

	//Обновление количества символов:
	number_of_chars = bufarray_bottom_number_of_chars + total_len_received + bufarray_top_number_of_chars;
}

//Функция освобождения выделенной под строки памяти при удалении графика:
static void
string_memory_allocate_graph_delete (unsigned short int graph_number)
{
	//Создание нижнего буфера:
	char *bufarray_bottom;
	unsigned int bufarray_bottom_number_of_strings = 2;
	unsigned int bufarray_bottom_number_of_chars = 0;
	unsigned int bufarray_bottom_size;
	//Создание верхнего буфера:
	char *bufarray_top;
	unsigned int bufarray_top_number_of_strings = 0;
	unsigned int bufarray_top_number_of_chars = 0;
	unsigned int bufarray_top_size;

	//Подсчёт количества строк, сохраняемых в нижнем буфере:
	for (unsigned short int i = 1; i < graph_number + 1; i++)
	{
		bufarray_bottom_number_of_strings
			+= graph_array[i - 1].graph.y_number_of_major_grids + 1
			+ graph_array[i - 1].graph.x_number_of_major_grids + 1
			+ 2;
	}
	//Подсчёт количества символов, сохраняемых в нижнем буфере:
	unsigned int addr_old;
	addr_old = (unsigned int) char_array;
	unsigned int len_old;
	for (unsigned int i = 1; i <= bufarray_bottom_number_of_strings; i++)
	{
		len_old = strlen ((char *) addr_old) + 1;
		bufarray_bottom_number_of_chars += len_old;
		addr_old += len_old * sizeof (char);
	}
	//Вычисление требуемого объёма памяти нижнего буфера:
	bufarray_bottom_size = bufarray_bottom_number_of_chars * sizeof (char);

	//Подсчёт количества строк, сохраняемых в верхнем буфере:
	for (unsigned short int i = graph_number + 1 + 1; i <= number_of_graphs; i++)
	{
		bufarray_top_number_of_strings
			+= graph_array[i - 1].graph.y_number_of_major_grids + 1
			+ graph_array[i - 1].graph.x_number_of_major_grids + 1
			+ 2;
	}
	//Подсчёт количества символов, сохраняемых в верхнем буфере:
	if (bufarray_top_number_of_strings)
		addr_old = (unsigned int) graph_array[graph_number + 1].graph.y_tick_labels;
	for (unsigned int i = 1; i <= bufarray_top_number_of_strings; i++)
	{
		len_old = strlen ((char *) addr_old) + 1;
		bufarray_top_number_of_chars += len_old;
		addr_old += len_old * sizeof (char);
	}
	//Вычисление требуемого объёма памяти нижнего буфера:
	bufarray_top_size = bufarray_top_number_of_chars * sizeof (char);

	//Выделение памяти под массивы временного хранения:
	bufarray_bottom = (char *) malloc (bufarray_bottom_size);
	bufarray_top = (char *) malloc (bufarray_top_size);
	//Копирование старых символов в массивы временного хранения:
	memcpy (bufarray_bottom, char_array, bufarray_bottom_size);
	memcpy (bufarray_top, graph_array[graph_number + 1].graph.y_tick_labels, bufarray_top_size);

	//Выделение нового количества памяти под массив символов:
	free (char_array);
	char_array = (char *) malloc (bufarray_bottom_size + bufarray_top_size);

	//Восстановление старых символов из массивов временного хранения:
	memcpy (char_array, bufarray_bottom, bufarray_bottom_size);
	memcpy ((char *) ((unsigned int) char_array + bufarray_bottom_size),
		bufarray_top,
		bufarray_top_size);

	//Освобождение памяти, выделенной под массивы временного хранения:
	free (bufarray_bottom);
	free (bufarray_top);

	//Адреса шрифтов и новых символов в массиве символов:
	unsigned int font_tl_addr_new;
	font_tl_addr_new = (unsigned int) char_array;
	unsigned int font_tl_len_new;
	font_tl_len_new = strlen ((char *) font_tl_addr_new) + 1;
	unsigned int font_al_addr_new;
	font_al_addr_new = font_tl_addr_new + font_tl_len_new * sizeof (char);
	unsigned int font_al_len_new;
	font_al_len_new = strlen ((char *) font_al_addr_new) + 1;

	//Обновление адресов имён шрифтов:
	graph_appearance.tick_labels_font.name = (char *) font_tl_addr_new;
	graph_appearance.axis_labels_font.name = (char *) font_al_addr_new;

	//Обновление адресов меток и подписей осей:
	unsigned int addr_new;
	unsigned int len_new;
	addr_new = font_al_addr_new + font_al_len_new * sizeof (char);
	for (unsigned short int i = 1; i <= number_of_graphs; i++)
	{
		if (i != graph_number + 1)
		{
			len_new = 0;
			//Обновление адресов меток оси Y:
			graph_array[i - 1].graph.y_tick_labels = (void *) addr_new;
			for (unsigned int j = 1; j <= graph_array[i - 1].graph.y_number_of_major_grids + 1; j++)
			{
				len_new = strlen ((char *) addr_new) + 1;
				addr_new += len_new * sizeof (char);
			}
			//Обновление адресов меток оси X:
			graph_array[i - 1].graph.x_tick_labels = (void *) addr_new;
			for (unsigned int j = 1; j <= graph_array[i - 1].graph.x_number_of_major_grids + 1; j++)
			{
				len_new = strlen ((char *) addr_new) + 1;
				addr_new += len_new * sizeof (char);
			}
			//Обновление адреса подписи оси Y:
			graph_array[i - 1].graph.y_axis_label = (void *) addr_new;
			len_new = strlen ((char *) addr_new) + 1;	
			addr_new += len_new * sizeof (char);
			//Обновление адреса подписи оси X:
			graph_array[i - 1].graph.x_axis_label = (void *) addr_new;
			len_new = strlen ((char *) addr_new) + 1;	
			addr_new += len_new * sizeof (char);
		}
	}

	//Обновление количества символов:
	number_of_chars = bufarray_bottom_number_of_chars + bufarray_top_number_of_chars;
}

//Функция выделения памяти под точки при создании нового графика:
static void
point_memory_allocate_graph_add ()
{
	//Выделение памяти под точки первого графика:
	if (number_of_points == 0)
	{
		//Выделение памяти под массив точек:
		point_array = (struct point *) malloc (graph_array[number_of_graphs - 1].graph.trace_number_of_points * sizeof (struct point));
		//Копирование новых точек в массив точек:
		memcpy (point_array,
			graph_array[number_of_graphs - 1].graph.trace_points,
			graph_array[number_of_graphs - 1].graph.trace_number_of_points * sizeof (struct point));

	//Выделение памяти под точки последующих графиков:
	} else {
		//Выделение памяти под массив временного хранения:
		struct point *bufarray_top;
		bufarray_top = (struct point *) malloc (number_of_points * sizeof (struct point));
		//Копирование старых точек в массив временного хранения:
		memcpy (bufarray_top, point_array, number_of_points * sizeof (struct point));
		//Выделение нового количества памяти под массив точек:
		free (point_array);
		point_array = (struct point *) malloc ((number_of_points + graph_array[number_of_graphs - 1].graph.trace_number_of_points) * sizeof (struct point));
		//Восстановление старых точек из массива временного хранения:
		memcpy (point_array, bufarray_top, number_of_points * sizeof (struct point));
		//Освобождение памяти, выделенной под массив временного хранения:
		free (bufarray_top);
		//Копирование новых точек в массив точек:
		memcpy ((struct point *) ((unsigned int) point_array + number_of_points * sizeof (struct point)),
			graph_array[number_of_graphs - 1].graph.trace_points,
			graph_array[number_of_graphs - 1].graph.trace_number_of_points * sizeof (struct point));
	}

	//Обновление адресов точек:
	unsigned int addr_new;
	addr_new = (unsigned int) point_array;
	for (unsigned short int i = 1; i <= number_of_graphs; i++)
	{
		graph_array[i - 1].graph.trace_points = (struct point *) addr_new;
		addr_new += graph_array[i - 1].graph.trace_number_of_points * sizeof (struct point);
	}
	//Обновление количества точек:
	number_of_points += graph_array[number_of_graphs - 1].graph.trace_number_of_points;
}

//Функция выделения памяти под точки при обновлении существующего графика:
static void
point_memory_allocate_graph_update (unsigned short int graph_number)
{
	//Создание нижнего буфера:
	struct point *bufarray_bottom;
	unsigned int bufarray_bottom_number_of_points = 0;
	unsigned int bufarray_bottom_size = 0;
	//Создание верхнего буфера:
	struct point *bufarray_top;
	unsigned int bufarray_top_number_of_points = 0;
	unsigned int bufarray_top_size = 0;
	//Определение потребности в массивах временного хранения:
	if (number_of_graphs > 1)
	{
		if ((graph_number + 1) > 1)
		{
			//Вычисление количества точек и требуемого объёма памяти:
			for (unsigned short int i = 1; i <= graph_number + 1 - 1; i++)
			{
				bufarray_bottom_number_of_points += graph_array[i - 1].graph.trace_number_of_points;
			}
			bufarray_bottom_size = bufarray_bottom_number_of_points * sizeof (struct point);
			//Выделение памяти под нижний буфер:
			bufarray_bottom = (struct point *) malloc (bufarray_bottom_size);
			//Копирование старых точек в нижний буфер:
			memcpy (bufarray_bottom, graph_array[0].graph.trace_points, bufarray_bottom_size);
		}
		if (graph_number + 1 < number_of_graphs)
		{
			//Вычисление количества точек и требуемого объёма памяти:
			for (unsigned short int i = graph_number + 1 + 1; i <= number_of_graphs; i++)
			{
				bufarray_top_number_of_points += graph_array[i - 1].graph.trace_number_of_points;
			}
			bufarray_top_size = bufarray_top_number_of_points * sizeof (struct point);
			//Выделение памяти под верхний буфер:
			bufarray_top = (struct point *) malloc (bufarray_top_size);
			//Копирование старых точек в верхний буфер:
			memcpy (bufarray_top, graph_array[graph_number + 1].graph.trace_points, bufarray_top_size);
		}
	}
	//Выделение нового количества памяти под массив точек:
	free (point_array);
	point_array = (struct point *) malloc (bufarray_bottom_size + (graph_array[graph_number].graph.trace_number_of_points) * sizeof (struct point) + bufarray_top_size);
	//Восстановление старых точек из массивов временного хранения:
	memcpy (point_array, bufarray_bottom, bufarray_bottom_size);
	memcpy ((struct point *) ((unsigned int) point_array + bufarray_bottom_size + (graph_array[graph_number].graph.trace_number_of_points) * sizeof (struct point)),
		bufarray_top,
		bufarray_top_size);
	//Освобождение памяти, выделенной под массивы временного хранения:
	if (bufarray_bottom_size) free (bufarray_bottom);
	if (bufarray_top_size) free (bufarray_top);
	//Копирование новых точек в массив точек:
	memcpy ((struct point *) ((unsigned int) point_array + bufarray_bottom_size),
		graph_array[graph_number].graph.trace_points,
		graph_array[graph_number].graph.trace_number_of_points * sizeof (struct point));

	//Обновление адресов точек:
	unsigned int addr_new;
	addr_new = (unsigned int) point_array;
	for (unsigned short int i = 1; i <= number_of_graphs; i++)
	{
		graph_array[i - 1].graph.trace_points = (struct point *) addr_new;
		addr_new += graph_array[i - 1].graph.trace_number_of_points * sizeof (struct point);
	}
	//Обновление количества точек:
	number_of_points = bufarray_bottom_number_of_points + graph_array[graph_number].graph.trace_number_of_points + bufarray_top_number_of_points;
}

//Функция освобождения выделенной под точки памяти при удалении графика:
static void
point_memory_allocate_graph_delete (unsigned short int graph_number)
{
	//Создание нижнего буфера:
	struct point *bufarray_bottom;
	unsigned int bufarray_bottom_number_of_points = 0;
	unsigned int bufarray_bottom_size = 0;
	//Создание верхнего буфера:
	struct point *bufarray_top;
	unsigned int bufarray_top_number_of_points = 0;
	unsigned int bufarray_top_size = 0;
	//Определение потребности в массивах временного хранения:
	if (number_of_graphs > 1)
	{
		if ((graph_number + 1) > 1)
		{
			//Вычисление количества точек и требуемого объёма памяти:
			for (unsigned short int i = 1; i <= graph_number + 1 - 1; i++)
			{
				bufarray_bottom_number_of_points += graph_array[i - 1].graph.trace_number_of_points;
			}
			bufarray_bottom_size = bufarray_bottom_number_of_points * sizeof (struct point);
			//Выделение памяти под нижний буфер:
			bufarray_bottom = (struct point *) malloc (bufarray_bottom_size);
			//Копирование старых точек в нижний буфер:
			memcpy (bufarray_bottom, graph_array[0].graph.trace_points, bufarray_bottom_size);
		}
		if (graph_number + 1 < number_of_graphs)
		{
			//Вычисление количества точек и требуемого объёма памяти:
			for (unsigned short int i = graph_number + 1 + 1; i <= number_of_graphs; i++)
			{
				bufarray_top_number_of_points += graph_array[i - 1].graph.trace_number_of_points;
			}
			bufarray_top_size = bufarray_top_number_of_points * sizeof (struct point);
			//Выделение памяти под верхний буфер:
			bufarray_top = (struct point *) malloc (bufarray_top_size);
			//Копирование старых точек в верхний буфер:
			memcpy (bufarray_top, graph_array[graph_number + 1].graph.trace_points, bufarray_top_size);
		}
	}
	//Выделение нового количества памяти под массив точек:
	free (point_array);
	point_array = (struct point *) malloc (bufarray_bottom_size + bufarray_top_size);
	//Восстановление старых точек из массивов временного хранения:
	memcpy (point_array, bufarray_bottom, bufarray_bottom_size);
	memcpy ((struct point *) ((unsigned int) point_array + bufarray_bottom_size),
		bufarray_top,
		bufarray_top_size);
	//Освобождение памяти, выделенной под массивы временного хранения:
	if (bufarray_bottom_size) free (bufarray_bottom);
	if (bufarray_top_size) free (bufarray_top);

	//Обновление адресов точек:
	unsigned int addr_new;
	addr_new = (unsigned int) point_array;
	for (unsigned short int i = 1; i <= number_of_graphs; i++)
	{
		if (i != graph_number + 1)
		{
			graph_array[i - 1].graph.trace_points = (struct point *) addr_new;
			addr_new += graph_array[i - 1].graph.trace_number_of_points * sizeof (struct point);
		}
	}
	//Обновление количества точек:
	number_of_points = bufarray_bottom_number_of_points + bufarray_top_number_of_points;
}*/

//Функция изменения настроек внешнего вида графиков:
void
graph_appearance_set (struct graph_appearance new_graph_appearance)
{
	graph_appearance = new_graph_appearance;
	string_memory_allocate_fonts_update ();
}

//Функция построения графика:
void
graph_plot (GtkDrawingArea *area, struct graph new_graph)
{
	//Проверка принятой области рисования на совпадение с имеющимися в массиве графиков:
	bool area_match = FALSE;
	unsigned short int match_number;
	for (unsigned short int i = 1; i <= number_of_graphs; i++)
	{
		if (area == graph_array[i - 1].area)
		{
			area_match = TRUE;
			match_number = i - 1;
		}
	}

	//Обновление существующего графика:
	if (area_match)
	{
		//Обновление элемента массива графиков:
		graph_array[match_number].graph = new_graph;
		//Выделение памяти под строки:
//		string_memory_allocate_graph_update (match_number);
		//Выделение памяти под точки:
//		point_memory_allocate_graph_update (match_number);
		//Обновление изображения:
		gtk_widget_hide (GTK_WIDGET (graph_array[match_number].area));
		gtk_widget_show (GTK_WIDGET (graph_array[match_number].area));

	//Создание нового графика:
	} else {
		//Обновление количества графиков:
		number_of_graphs++;
		//Отключение старых сигналов:
		if (number_of_graphs > 1)
		{
			for (unsigned short int i = 1; i <= number_of_graphs - 1; i++)
			{
				g_signal_handlers_disconnect_by_func (graph_array[i - 1].area, (GFunc) configure_event_cb, &graph_array[i - 1].number);
				g_signal_handlers_disconnect_by_func (graph_array[i - 1].area, (GFunc) draw_cb, &graph_array[i - 1].number);
				g_signal_handlers_disconnect_by_func (graph_array[i - 1].area, (GFunc) size_allocate_cb, &graph_array[i - 1].number);
			}
		}
		//Выделение нового количества памяти под массив графиков:
		graph_array = (struct graph_internal *) realloc (graph_array, number_of_graphs * sizeof (struct graph_internal));
		//Заполнение нового элемента массива графиков:
		graph_array[number_of_graphs - 1].number = number_of_graphs - 1;
		graph_array[number_of_graphs - 1].area = area;
		GtkRequisition minimum_size;
		gtk_widget_get_preferred_size (GTK_WIDGET (area), &minimum_size, NULL);
		graph_array[number_of_graphs - 1].area_height_request = minimum_size.height;
		graph_array[number_of_graphs - 1].area_width_request = minimum_size.width;
		cairo_surface_t *surface = NULL;
		graph_array[number_of_graphs - 1].surface = surface;
		cairo_t *context = NULL;
		graph_array[number_of_graphs - 1].context = context;
		graph_array[number_of_graphs - 1].graph = new_graph;
		//Выделение памяти под новые строки:
		string_memory_allocate_graph_add ();
		//Выделение памяти под новые точки:
//		point_memory_allocate_graph_add ();
		//Подключение новых сигналов:
		for (unsigned short int i = 1; i <= number_of_graphs; i++)
		{
			g_signal_connect (graph_array[i - 1].area, "configure-event", G_CALLBACK (configure_event_cb), &graph_array[i - 1].number);
			g_signal_connect (graph_array[i - 1].area, "draw", G_CALLBACK (draw_cb), &graph_array[i - 1].number);
			g_signal_connect (graph_array[i - 1].area, "size-allocate", G_CALLBACK (size_allocate_cb), &graph_array[i - 1].number);
		}
		//Обновление изображения:
		gtk_widget_hide (GTK_WIDGET (graph_array[number_of_graphs - 1].area));
		gtk_widget_show (GTK_WIDGET (graph_array[number_of_graphs - 1].area));
	}
}

//Функция удаления графика:
void
graph_delete (GtkDrawingArea *area)
{
	//Проверка принятой области рисования на совпадение с имеющимися в массиве графиков:
	bool area_match = FALSE;
	unsigned short int match_number;
	for (unsigned short int i = 1; i <= number_of_graphs; i++)
	{
		if (area == graph_array[i - 1].area)
		{
			area_match = TRUE;
			match_number = i - 1;
		}
	}

	//Удаление существующего графика:
	if (area_match)
	{
		//Отключение старых сигналов:
		for (unsigned short int i = 1; i <= number_of_graphs; i++)
		{
			g_signal_handlers_disconnect_by_func (graph_array[i - 1].area, (GFunc) configure_event_cb, &graph_array[i - 1].number);
			g_signal_handlers_disconnect_by_func (graph_array[i - 1].area, (GFunc) draw_cb, &graph_array[i - 1].number);
			g_signal_handlers_disconnect_by_func (graph_array[i - 1].area, (GFunc) size_allocate_cb, &graph_array[i - 1].number);
		}
		//Уничтожение поверхности Cairo:
		if (graph_array[match_number].surface)
			cairo_surface_destroy (graph_array[match_number].surface);
		//Обновление минимальных размеров области рисования:
		gint area_height_request = graph_array[match_number].area_height_request;
		gint area_width_request = graph_array[match_number].area_width_request;
		gtk_widget_set_size_request (GTK_WIDGET (graph_array[match_number].area), area_width_request, area_height_request);
		//Освобождение памяти, выделенной под строки удаляемого графика:
//		string_memory_allocate_graph_delete (match_number);
		//Освобождение памяти, выделенной под точки удаляемого графика:
//		point_memory_allocate_graph_delete (match_number);
		//Обновление массива графиков:
		for (unsigned short int i = match_number + 1; i < number_of_graphs; i++)
		{
			graph_array[i - 1].area = graph_array[i].area;
			graph_array[i - 1].area_height_request = graph_array[i].area_height_request;
			graph_array[i - 1].area_width_request = graph_array[i].area_width_request;
			graph_array[i - 1].surface = graph_array[i].surface;
			graph_array[i - 1].context = graph_array[i].context;
			graph_array[i - 1].graph = graph_array[i].graph;
		}
		//Выделение нового количества памяти под массив графиков:
		graph_array = (struct graph_internal *) realloc (graph_array, (number_of_graphs - 1) * sizeof (struct graph_internal));
		//Подключение новых сигналов:
		for (unsigned short int i = 1; i < number_of_graphs; i++)
		{
			g_signal_connect (graph_array[i - 1].area, "configure-event", G_CALLBACK (configure_event_cb), &graph_array[i - 1].number);
			g_signal_connect (graph_array[i - 1].area, "draw", G_CALLBACK (draw_cb), &graph_array[i - 1].number);
			g_signal_connect (graph_array[i - 1].area, "size-allocate", G_CALLBACK (size_allocate_cb), &graph_array[i - 1].number);
		}
		//Обновление количества графиков:
		number_of_graphs--;
	}
}

//Функция освобождения памяти, выделенной под графики, строки и точки:
void
graph_memory_free ()
{
	for (unsigned short int i = 0; i <= number_of_graphs - 1; i++)
	{
		//Отключение сигналов:
		g_signal_handlers_disconnect_by_func (graph_array[i].area, (GFunc) configure_event_cb, &graph_array[i].number);
		g_signal_handlers_disconnect_by_func (graph_array[i].area, (GFunc) draw_cb, &graph_array[i].number);
		g_signal_handlers_disconnect_by_func (graph_array[i].area, (GFunc) size_allocate_cb, &graph_array[i].number);
		//Уничтожение поверхности Cairo:
		if (graph_array[i].surface)
			cairo_surface_destroy (graph_array[i].surface);
		//Обновление минимальных размеров области рисования:
		gtk_widget_set_size_request (GTK_WIDGET (graph_array[i].area),
			graph_array[i].area_height_request,
			graph_array[i].area_width_request);
	}
	if (number_of_graphs)
	{
		//Освобождение памяти, выделенной под массив графиков:
		free (graph_array);
		//Обнуление количества графиков:
		number_of_graphs = 0;
	}
	if (number_of_chars)
	{
		//Освобождение памяти, выделенной под массив символов:
		free (char_array);
		//Обнуление количества символов:
		number_of_chars = 0;
	}
	if (number_of_points)
	{
		//Освобождение памяти, выделенной под массив точек:
		free (point_array);
		//Обнуление количества точек:
		number_of_points = 0;
	}
}










//Функция очистки области рисования:
static void
clear_graph (unsigned short int graph_number)
{
	graph_array[graph_number].context = cairo_create (graph_array[graph_number].surface);
	cairo_set_source_rgb (graph_array[graph_number].context,
		graph_appearance.background_color.r,
		graph_appearance.background_color.g,
		graph_appearance.background_color.b);
	cairo_paint (graph_array[graph_number].context);
	cairo_destroy (graph_array[graph_number].context);
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
plot_minor_gridlines (unsigned short int graph_number, struct geometry coordinate_plane_geometry, bool y_minor_gridlines, bool x_minor_gridlines)
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
plot_major_gridlines (unsigned short int graph_number, struct geometry coordinate_plane_geometry)
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

//!!!Не доделан масштаб
//!!!Функция вычерчивания трассы:
static void
plot_trace (unsigned short int graph_number, struct geometry coordinate_plane_geometry)
{
	//Начальные установки:
	cairo_set_line_cap (graph_array[graph_number].context, CAIRO_LINE_CAP_ROUND);
	cairo_set_line_join (graph_array[graph_number].context, CAIRO_LINE_JOIN_ROUND);
	cairo_set_line_width (graph_array[graph_number].context, graph_appearance.traces_thickness);
	cairo_set_source_rgb (graph_array[graph_number].context, graph_appearance.traces_color.r, graph_appearance.traces_color.g, graph_appearance.traces_color.b);

//Вычерчивание трассы:


//ЛИНЛИН
if ((graph_array[graph_number].graph.x_scale == LIN) && (graph_array[graph_number].graph.y_scale == LIN))
{
	//!!!Сейчас пишу для линейного масштаба по обеим осям. Для одной трассы. Точки разрыва не предусмотрены. Строится по введённым точкам (кусочно-линейная аппроксимация)
	//!Здесь целесообразно перейти к порядку x - y от порядка y - x, так как функция move
	//!Расстояние по X в числах:
	double width_value = graph_array[graph_number].graph.x_value_max - graph_array[graph_number].graph.x_value_min;
	//!Расстояние по Y в числах:
	double height_value = graph_array[graph_number].graph.y_value_max - graph_array[graph_number].graph.y_value_min;
	//!Координаты краёв координатной плоскости берутся из coordinate_plane_geometry
	//!Координаты текущей точки:
	guint point_x_coordinate;
	guint point_y_coordinate;
	//!Вычисление первой позиции курсора:
	point_x_coordinate = coordinate_plane_geometry.x + (((graph_array[graph_number].graph.trace_points[0].x - graph_array[graph_number].graph.x_value_min) / width_value) * coordinate_plane_geometry.width);
	point_y_coordinate = coordinate_plane_geometry.y - (((graph_array[graph_number].graph.trace_points[0].y - graph_array[graph_number].graph.y_value_min) / height_value) * coordinate_plane_geometry.height);
	//!Перемещение к первой точке:
	cairo_move_to (graph_array[graph_number].context, point_x_coordinate, point_y_coordinate);
	//!Построение оставшихся точек:
	for (unsigned int i = 2; i <= graph_array[graph_number].graph.trace_number_of_points; i++)
	{
		point_x_coordinate = coordinate_plane_geometry.x + (((graph_array[graph_number].graph.trace_points[i - 1].x - graph_array[graph_number].graph.x_value_min) / width_value) * coordinate_plane_geometry.width);
		point_y_coordinate = coordinate_plane_geometry.y - (((graph_array[graph_number].graph.trace_points[i - 1].y - graph_array[graph_number].graph.y_value_min) / height_value) * coordinate_plane_geometry.height);
		cairo_line_to (graph_array[graph_number].context, point_x_coordinate, point_y_coordinate);
	}
}

//ЛИН по X ЛОГ по Y//Последовательность вариантов вроде логичная
if ((graph_array[graph_number].graph.x_scale == LIN) && (graph_array[graph_number].graph.y_scale == LOG))
{
}


//Не строит.
//ЛОГ по X ЛИН по Y
if ((graph_array[graph_number].graph.x_scale == LOG) && (graph_array[graph_number].graph.y_scale == LIN))
{
	//!Расстояние по Y в числах:
	double height_value = graph_array[graph_number].graph.y_value_max - graph_array[graph_number].graph.y_value_min;
	//!!!Количество сеток X:
	//Если здесь unsigned int то не строит. Если Double, то строит
	//Еслли округлять с помощью ceil (X - 0.5), то строит. Но Axis Label отобразил не корректо
	//Последний Xtick объединяется с Xaxis. ПРИ ЛИНЕЙНОМ МАСШТАБЕ ТАКЖЕ. ПРОБЛЕМА В ДРУГОМ
	unsigned int XNoG = ceil (log10 (graph_array[graph_number].graph.x_value_max / graph_array[graph_number].graph.x_value_min) - 0.5);	//Тип под вопросом
	//double XNoG = log10 (graph_array[graph_number].graph.x_value_max / graph_array[graph_number].graph.x_value_min);	
	//!Координаты краёв координатной плоскости берутся из coordinate_plane_geometry
	//!Координаты текущей точки:
	guint point_x_coordinate;
	guint point_y_coordinate;
	//!Вычисление первой позиции курсора:
	point_x_coordinate = coordinate_plane_geometry.x + ((log10 (graph_array[graph_number].graph.trace_points[0].x / graph_array[graph_number].graph.x_value_min) / XNoG) * coordinate_plane_geometry.width);
	point_y_coordinate = coordinate_plane_geometry.y - (((graph_array[graph_number].graph.trace_points[0].y - graph_array[graph_number].graph.y_value_min) / height_value) * coordinate_plane_geometry.height);
	//!Перемещение к первой точке:
	cairo_move_to (graph_array[graph_number].context, point_x_coordinate, point_y_coordinate);
	//!Построение оставшихся точек:
	for (unsigned int i = 2; i <= graph_array[graph_number].graph.trace_number_of_points; i++)
	{
		point_x_coordinate = coordinate_plane_geometry.x + ((log10 (graph_array[graph_number].graph.trace_points[i - 1].x / graph_array[graph_number].graph.x_value_min) / XNoG) * coordinate_plane_geometry.width);
		point_y_coordinate = coordinate_plane_geometry.y - (((graph_array[graph_number].graph.trace_points[i - 1].y - graph_array[graph_number].graph.y_value_min) / height_value) * coordinate_plane_geometry.height);
		cairo_line_to (graph_array[graph_number].context, point_x_coordinate, point_y_coordinate);
	}
}


//ЛОГЛОГ
if ((graph_array[graph_number].graph.x_scale == LOG) && (graph_array[graph_number].graph.y_scale == LOG))
{
}



//!!!НУЖНО ОГРАНИЧИТЬ ОБЛАСТЬ РИСОВАНИЯ, чтобы трасса не наслаивалась на границы координатной плоскости и не выходила за её пределы

	cairo_stroke (graph_array[graph_number].context);
}

//!!!Нужно переделать работу с tick_labels
//Функция вычерчивания меток осей:
static void
plot_tick_labels (unsigned short int graph_number, struct geometry *y_tick_labels_geometry, struct geometry *x_tick_labels_geometry)
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
plot_axis_labels (unsigned short int graph_number, struct geometry y_axis_label_geometry, struct geometry x_axis_label_geometry)
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

//!!!Нужно переделать работу с tick_labels
//Функция вычерчивания графика:
static void
plot_graph (unsigned short int graph_number, guint area_height, guint area_width)
{
	//Очистка области рисования:
	clear_graph (graph_number);

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

	//Построение графика:
	plot_minor_gridlines (graph_number, coordinate_plane_geometry, y_minor_gridlines, x_minor_gridlines);
	plot_major_gridlines (graph_number, coordinate_plane_geometry);
	plot_trace (graph_number, coordinate_plane_geometry);
	plot_tick_labels (graph_number, y_tick_labels_geometry, x_tick_labels_geometry);
	plot_axis_labels (graph_number, y_axis_label_geometry, x_axis_label_geometry);

	cairo_destroy (graph_array[graph_number].context);

	gtk_widget_queue_draw_area (GTK_WIDGET (graph_array[graph_number].area), 0, 0, area_width, area_height);
}

//!!!Временная функция вывода всего массива символов:
void char_array_print ()
{
	g_print ("Печать массива символов:\n");
	for (unsigned int i = 0; i < number_of_chars; i++)
	{
		if (char_array[i] == '\0')
		{
			g_print ("\tСимвол №%i: .конец строки.\n", i);
		} else {
			g_print ("\tСимвол №%i: .%c.\n", i, char_array[i]);
		}
	}
}
