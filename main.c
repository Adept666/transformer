//Программа для расчёта трансформатора обратноходового ИВЭП
//v0.036.
//!!!!!!!ОБЛАСТИ РИСОВАНИЯ РАЗНЫХ ГРАФИКОВ ДОЛЖНЫ БЫТЬ РАЗНЫМИ
//!!!!!!!При обновлении графика с существующим номером область рисования ДОЛЖНА ОСТАТЬСЯ ПРЕЖНЕЙ

//Файл переименован в main.c
//!!!Нужно реализовать вывод графиков (Pv, Кривые Дауэлла)
//!!! У КРУГЛЫХ КАРКАСОВ НИЖНЯЯ ЧАСТЬ ОБМОТКИ УЧИТЫВАЕТСЯ!!! ПО ДРУГОМУ И НЕ ПОЛУЧИТСЯ
//!!!Рабочий цикл можно выбирать на основе шим контроллера и его мёртвого времени
//!!!В файлах нужна возможность оставлять комментарии
//!!!При выборе деталей в лист боксах должен быть пункт "не выбрано". Тогда программа спросит по ходу
//!!!НУЖНО ВВЕСТИ ПРОВЕРКУ ТОГО, ЧТО МАКСИМАЛЬНОЕ ЗНАЧЕНИЕ БОЛЬШЕ НОМИНАЛЬНОГО И МИНИМАЛЬНОГО И ТД
#include <gtk/gtk.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "graph.h"	//(36)

//Виджеты entry для ввода исходных данных:
GtkWidget *entry_input_vout_min, *entry_input_vout_nom, *entry_input_vout_max, *entry_input_vout_tol;
GtkWidget *entry_input_iout_min, *entry_input_iout_nom, *entry_input_iout_max, *entry_input_iout_tol;
GtkWidget *entry_input_eta_min, *entry_input_eta_nom, *entry_input_eta_max, *entry_input_eta_tol;
GtkWidget *entry_input_f_min, *entry_input_f_nom, *entry_input_f_max, *entry_input_f_tol;
GtkWidget *entry_input_d_min, *entry_input_d_nom, *entry_input_d_max, *entry_input_d_tol;
GtkWidget *entry_input_t_min, *entry_input_t_nom, *entry_input_t_max, *entry_input_t_tol;
GtkWidget *entry_input_vin_min, *entry_input_vin_nom, *entry_input_vin_max, *entry_input_vin_tol;

//Глобальные переменные для хранения исходных данных:
//Используется система СИ
double Vout_min, Vout_nom, Vout_max, Vout_tol;
double Iout;
double eta;
double f_min, f_nom, f_max, f_tol;
double D_max_min, D_max_max;
double t_min, t_max;
double Vin_min, Vin_nom, Vin_max, Vin_tol;

GtkWidget *drawing_area_graph1;	//(36)
GtkWidget *drawing_area_graph2;	//(36)
GtkWidget *drawing_area_graph3;	//(36)

//Консоль, её буфер, итератор и метка:
GtkWidget *text_view_console;
GtkTextBuffer *text_buffer_console;
GtkTextIter text_iter_console;
GtkTextMark *text_mark_console;

//Функция вывода строки в консоль:
static void console_print(char *line)
{
	//Вывод аргумента и символа переноса строки:
	gtk_text_buffer_insert(text_buffer_console, &text_iter_console, line, -1);
	gtk_text_buffer_insert(text_buffer_console, &text_iter_console, "\n", -1);
	
	//Прокрутка до последней записи:
	text_mark_console = gtk_text_buffer_create_mark(text_buffer_console, NULL, &text_iter_console, FALSE);
	gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(text_view_console), text_mark_console);
	gtk_text_buffer_delete_mark(text_buffer_console, text_mark_console);
	while (gtk_events_pending ()) gtk_main_iteration();
}

//Функция записи исходных данных в глобальные переменные:
static void input_data_get()
{
	//Выходное напряжение (В);
	Vout_min = strtod(gtk_entry_get_text(GTK_ENTRY(entry_input_vout_min)), NULL);
	Vout_nom = strtod(gtk_entry_get_text(GTK_ENTRY(entry_input_vout_nom)), NULL);
	Vout_max = strtod(gtk_entry_get_text(GTK_ENTRY(entry_input_vout_max)), NULL);
	Vout_tol = strtod(gtk_entry_get_text(GTK_ENTRY(entry_input_vout_tol)), NULL);
	//Ток нагрузки (А);
	Iout = strtod(gtk_entry_get_text(GTK_ENTRY(entry_input_iout_max)), NULL);
	//КПД (безразмерная величина);
	eta = strtod(gtk_entry_get_text(GTK_ENTRY(entry_input_eta_nom)), NULL);
	//Частота преобразования (кГц -> Гц);
	f_min = 1e+3 * strtod(gtk_entry_get_text(GTK_ENTRY(entry_input_f_min)), NULL);
	f_nom = 1e+3 * strtod(gtk_entry_get_text(GTK_ENTRY(entry_input_f_nom)), NULL);
	f_max = 1e+3 * strtod(gtk_entry_get_text(GTK_ENTRY(entry_input_f_max)), NULL);
	f_tol = 1e+3 * strtod(gtk_entry_get_text(GTK_ENTRY(entry_input_f_tol)), NULL);
	//Максимальный рабочий цикл (безразмерная величина);
	D_max_min = strtod(gtk_entry_get_text(GTK_ENTRY(entry_input_d_min)), NULL);
	D_max_max = strtod(gtk_entry_get_text(GTK_ENTRY(entry_input_d_max)), NULL);
	//Температура окружающей среды (°C);
	t_min = strtod(gtk_entry_get_text(GTK_ENTRY(entry_input_t_min)), NULL);
	t_max = strtod(gtk_entry_get_text(GTK_ENTRY(entry_input_t_max)), NULL);
	//Действующее значение входного напряжения (В);
	Vin_min = strtod(gtk_entry_get_text(GTK_ENTRY(entry_input_vin_min)), NULL);
	Vin_nom = strtod(gtk_entry_get_text(GTK_ENTRY(entry_input_vin_nom)), NULL);
	Vin_max = strtod(gtk_entry_get_text(GTK_ENTRY(entry_input_vin_max)), NULL);
	Vin_tol = strtod(gtk_entry_get_text(GTK_ENTRY(entry_input_vin_tol)), NULL);
}

//Функция чтения исходных данных из файла:
static void file_open()
{
	char file_line[256];
	FILE *file_input;
	file_input = fopen("data0034.trn", "r");
	if (file_input == NULL)
	{
		//Если файл не удалось открыть для чтения:
		console_print("Не удалось считать данные из файла");
	} else {
		//Если файл удалось открыть для чтения:
		fgets(file_line, 256, file_input);
		file_line[strlen(file_line) - 1] = 0;
		gtk_entry_set_text(GTK_ENTRY(entry_input_vout_min), file_line);
		fgets(file_line, 256, file_input);
		file_line[strlen(file_line) - 1] = 0;
		gtk_entry_set_text(GTK_ENTRY(entry_input_vout_nom), file_line);
		fgets(file_line, 256, file_input);
		file_line[strlen(file_line) - 1] = 0;
		gtk_entry_set_text(GTK_ENTRY(entry_input_vout_max), file_line);
		fgets(file_line, 256, file_input);
		file_line[strlen(file_line) - 1] = 0;
		gtk_entry_set_text(GTK_ENTRY(entry_input_vout_tol), file_line);
		fgets(file_line, 256, file_input);
		file_line[strlen(file_line) - 1] = 0;
		gtk_entry_set_text(GTK_ENTRY(entry_input_iout_max), file_line);
		fgets(file_line, 256, file_input);
		file_line[strlen(file_line) - 1] = 0;
		gtk_entry_set_text(GTK_ENTRY(entry_input_eta_nom), file_line);
		fgets(file_line, 256, file_input);
		file_line[strlen(file_line) - 1] = 0;
		gtk_entry_set_text(GTK_ENTRY(entry_input_f_min), file_line);
		fgets(file_line, 256, file_input);
		file_line[strlen(file_line) - 1] = 0;
		gtk_entry_set_text(GTK_ENTRY(entry_input_f_nom), file_line);
		fgets(file_line, 256, file_input);
		file_line[strlen(file_line) - 1] = 0;
		gtk_entry_set_text(GTK_ENTRY(entry_input_f_max), file_line);
		fgets(file_line, 256, file_input);
		file_line[strlen(file_line) - 1] = 0;
		gtk_entry_set_text(GTK_ENTRY(entry_input_f_tol), file_line);
		fgets(file_line, 256, file_input);
		file_line[strlen(file_line) - 1] = 0;
		gtk_entry_set_text(GTK_ENTRY(entry_input_d_min), file_line);
		fgets(file_line, 256, file_input);
		file_line[strlen(file_line) - 1] = 0;
		gtk_entry_set_text(GTK_ENTRY(entry_input_d_max), file_line);
		fgets(file_line, 256, file_input);
		file_line[strlen(file_line) - 1] = 0;
		gtk_entry_set_text(GTK_ENTRY(entry_input_t_min), file_line);
		fgets(file_line, 256, file_input);
		file_line[strlen(file_line) - 1] = 0;
		gtk_entry_set_text(GTK_ENTRY(entry_input_t_max), file_line);
		fgets(file_line, 256, file_input);
		file_line[strlen(file_line) - 1] = 0;
		gtk_entry_set_text(GTK_ENTRY(entry_input_vin_min), file_line);
		fgets(file_line, 256, file_input);
		file_line[strlen(file_line) - 1] = 0;
		gtk_entry_set_text(GTK_ENTRY(entry_input_vin_nom), file_line);
		fgets(file_line, 256, file_input);
		file_line[strlen(file_line) - 1] = 0;
		gtk_entry_set_text(GTK_ENTRY(entry_input_vin_max), file_line);
		fgets(file_line, 256, file_input);
		file_line[strlen(file_line) - 1] = 0;
		gtk_entry_set_text(GTK_ENTRY(entry_input_vin_tol), file_line);
		fclose(file_input);
	}
}

//Функция записи исходных данных в файл:
static void file_save()
{
	FILE *file_input;
	file_input = fopen("data0034.trn", "w");
	if (file_input == NULL)
	{
		//Если файл не удалось открыть для записи:
		console_print("Не удалось сохранить данные в файл");
	} else {
		//Если файл удалось открыть для записи:
		fprintf(file_input, "%s\n", gtk_entry_get_text(GTK_ENTRY(entry_input_vout_min)));
		fprintf(file_input, "%s\n", gtk_entry_get_text(GTK_ENTRY(entry_input_vout_nom)));
		fprintf(file_input, "%s\n", gtk_entry_get_text(GTK_ENTRY(entry_input_vout_max)));
		fprintf(file_input, "%s\n", gtk_entry_get_text(GTK_ENTRY(entry_input_vout_tol)));
		fprintf(file_input, "%s\n", gtk_entry_get_text(GTK_ENTRY(entry_input_iout_max)));
		fprintf(file_input, "%s\n", gtk_entry_get_text(GTK_ENTRY(entry_input_eta_nom)));
		fprintf(file_input, "%s\n", gtk_entry_get_text(GTK_ENTRY(entry_input_f_min)));
		fprintf(file_input, "%s\n", gtk_entry_get_text(GTK_ENTRY(entry_input_f_nom)));
		fprintf(file_input, "%s\n", gtk_entry_get_text(GTK_ENTRY(entry_input_f_max)));
		fprintf(file_input, "%s\n", gtk_entry_get_text(GTK_ENTRY(entry_input_f_tol)));
		fprintf(file_input, "%s\n", gtk_entry_get_text(GTK_ENTRY(entry_input_d_min)));
		fprintf(file_input, "%s\n", gtk_entry_get_text(GTK_ENTRY(entry_input_d_max)));
		fprintf(file_input, "%s\n", gtk_entry_get_text(GTK_ENTRY(entry_input_t_min)));
		fprintf(file_input, "%s\n", gtk_entry_get_text(GTK_ENTRY(entry_input_t_max)));
		fprintf(file_input, "%s\n", gtk_entry_get_text(GTK_ENTRY(entry_input_vin_min)));
		fprintf(file_input, "%s\n", gtk_entry_get_text(GTK_ENTRY(entry_input_vin_nom)));
		fprintf(file_input, "%s\n", gtk_entry_get_text(GTK_ENTRY(entry_input_vin_max)));
		fprintf(file_input, "%s\n", gtk_entry_get_text(GTK_ENTRY(entry_input_vin_tol)));
		fclose(file_input);
	}
}

//Функция, вызываемая при закрытии окна:(36)
static void window_destroy_cb()
{
	graph_memory_free();
	gtk_main_quit();
}

//Функция отклика главного меню:
static void menu_main_response(GtkWidget *menu_item, gpointer data)
{
	if (data == "Open") file_open();
	if (data == "Save") file_save();
}

//Функция запуска расчёта:
static void start_calculation(GtkWidget *button, gpointer data)
{
	struct graph graph1;	//(36)
	graph1.y_scale = LIN;
	graph1.x_scale = LIN;
	graph(GTK_DRAWING_AREA(drawing_area_graph1), graph1);	//(36)

	struct graph graph3;	//(36)
	graph3.y_scale = LIN;
	graph3.x_scale = LIN;
	graph(GTK_DRAWING_AREA(drawing_area_graph2), graph3);	//(36)

	//Вывод сообщения о запуске расчёта:
	console_print("Расчёт запущен");

	//Запись исходных данных в глобальные переменные:
	input_data_get();

	//Вывод данных:
	char console_line[256];	//Протяжённость строки консоли

	//Выходное напряжение ИВЭП:
	console_print("Выходное напряжение ИВЭП:");
	sprintf(console_line, "Vout_min = %g В", Vout_min);
	console_print(console_line);
	sprintf(console_line, "Vout_nom = %g В", Vout_nom);
	console_print(console_line);
	sprintf(console_line, "Vout_max = %g В", Vout_max);
	console_print(console_line);

	//Максимальный ток нагрузки ИВЭП:
	console_print("Максимальный ток нагрузки ИВЭП:");
	sprintf(console_line, "Iout = %g А", Iout);
	console_print(console_line);

	//Выходная мощность ИВЭП:
	console_print("Выходная мощность ИВЭП:");
	double Pout_min, Pout_nom, Pout_max;
	Pout_min = Vout_min * Iout;
	sprintf(console_line, "Pout_min = %g Вт", Pout_min);
	console_print(console_line);
	Pout_nom = Vout_nom * Iout;
	sprintf(console_line, "Pout_nom = %g Вт", Pout_nom);
	console_print(console_line);
	Pout_max = Vout_max * Iout;
	sprintf(console_line, "Pout_max = %g Вт", Pout_max);
	console_print(console_line);

	//Ожидаемое значение КПД ИВЭП:
	console_print("Ожидаемое значение КПД ИВЭП:");
	sprintf(console_line, "η = %g", eta);
	console_print(console_line);

	//Частота преобразования:
	console_print("Частота преобразования:");
	sprintf(console_line, "f_min = %g кГц", 1e-3 * f_min);
	console_print(console_line);
	sprintf(console_line, "f_nom = %g кГц", 1e-3 * f_nom);
	console_print(console_line);
	sprintf(console_line, "f_max = %g кГц", 1e-3 * f_max);
	console_print(console_line);

	//Период преобразования:
	console_print("Период преобразования:");
	double T_min, T_nom, T_max;
	T_min = 1 / f_max;
	sprintf(console_line, "T_min = %g мкс", 1e+6 * T_min);
	console_print(console_line);
	T_nom = 1 / f_nom;
	sprintf(console_line, "T_nom = %g мкс", 1e+6 * T_nom);
	console_print(console_line);
	T_max = 1 / f_min;
	sprintf(console_line, "T_max = %g мкс", 1e+6 * T_max);
	console_print(console_line);

	//Температура окружающей среды:
	console_print("Температура окружающей среды:");
	sprintf(console_line, "t_min = %g°C", t_min);
	console_print(console_line);
	sprintf(console_line, "t_max = %g°C", t_max);
	console_print(console_line);

	//Напряжение электроосветительной сети (действующее значение):
	console_print("Напряжение электросветительной сети (действующее значение):");
	sprintf(console_line, "Vin_min = %g В", Vin_min);
	console_print(console_line);
	sprintf(console_line, "Vin_nom = %g В", Vin_nom);
	console_print(console_line);
	sprintf(console_line, "Vin_max = %g В", Vin_max);
	console_print(console_line);

	//Постоянное напряжение на выходе НЧ выпрямителя:
	console_print("Постоянное напряжение на выходе НЧ выпрямителя:");
	double Vdc_min, Vdc_nom, Vdc_max;
	Vdc_min = sqrt(2.) * Vin_min;
	sprintf(console_line, "Vdc_min = %g В", Vdc_min);
	console_print(console_line);
	Vdc_nom = sqrt(2.) * Vin_nom;
	sprintf(console_line, "Vdc_nom = %g В", Vdc_nom);
	console_print(console_line);
	Vdc_max = sqrt(2.) * Vin_max;
	sprintf(console_line, "Vdc_max = %g В", Vdc_max);
	console_print(console_line);

	//Максимальный коэффициент заполнения импульсов ШИМ-контроллера:
	//!!!В дальнейшем данные переменные будут читаться из файла при выборе ШИМ-контроллера
	//!!!Вероятно, придётся учитывать "мёртвое время"
	console_print("Максимальный коэффициент заполнения импульсов ШИМ-контроллера");
	sprintf(console_line, "D_max_min = %g", D_max_min);
	console_print(console_line);
	sprintf(console_line, "D_max_max = %g", D_max_max);
	console_print(console_line);

	//Требуемое время прямого хода:
	console_print("Требуемое время прямого хода:");
	double tpri_min, tpri_nom, tpri_max;
	tpri_min = T_min * D_max_min;	//для расчётов
	sprintf(console_line, "tpri_min ≤ %g мкс", 1e+6 * tpri_min);
	console_print(console_line);
	tpri_nom = T_nom * D_max_min;
	sprintf(console_line, "tpri_nom ≤ %g мкс", 1e+6 * tpri_nom);
	console_print(console_line);
	tpri_max = T_max * D_max_min;
	sprintf(console_line, "tpri_max ≤ %g мкс", 1e+6 * tpri_max);
	console_print(console_line);

	//Требуемое время обратного хода:
	console_print("Требуемое время обратного хода:");
	double tsec_min, tsec_nom, tsec_max;
	tsec_min = T_min * (1 - D_max_max); //мин - для расчёта L2
	sprintf(console_line, "tsec_min ≤ %g мкс", 1e+6 * tsec_min);
	console_print(console_line);
	tsec_nom = T_nom * (1 - D_max_max);	//ном
	sprintf(console_line, "tsec_nom ≤ %g мкс", 1e+6 * tsec_nom);
	console_print(console_line);
	tsec_max = T_max * (1 - D_max_max);	//макс
	sprintf(console_line, "tsec_max ≤ %g мкс", 1e+6 * tsec_max);
	console_print(console_line);

	//Требуемая (максимальная) индуктивность первичной обмотки при наименее благоприятных условиях:
	//Максимальная выходная мощность ИВЭП Pout_max берётся с запасом в 20%
	//Под eta подразумевается ожидаемый КПД всей системы
	console_print("Требуемая индуктивность первичной обмотки при наименее благоприятных условиях:");
	double Lpri;
	Lpri = (pow(Vdc_min, 2.) * pow(tpri_min, 2.) * eta * f_max) / (2 * 1.2 * Pout_max);
	sprintf(console_line, "Lpri ≤ %g мкГн", 1e+6 * Lpri);
	console_print(console_line);

	//Падение напряжения на выходном диоде Шоттки:
	console_print("Падение напряжения на диоде Шоттки ВЧ выпрямителя:");
	double Vdout;
	Vdout = 0.5;
	sprintf(console_line, "Vdout = %g В", Vdout);
	console_print(console_line);

	//Требуемая (максимальная) индуктивность вторичной обмотки при наименее благоприятных условиях:
	//Ток нагрузки ИВЭП Iout берётся с запасом в 20%
	console_print("Требуемая индуктивность вторичной обмотки при наименее благоприятных условиях:");
	double Lsec;
	Lsec = (pow(Vout_max + Vdout, 2.) * pow(tsec_min, 2.) * f_max * eta) / (2 * 1.2 * Pout_max);
	sprintf(console_line, "Lsec ≤ %g мкГн", 1e+6 * Lsec);
	console_print(console_line);

	//Вывод разделителя:
	console_print("- - - - - - - - - -");

//!!!Здесь нужно выбрать тип феррита (Epcos N87) и размер каркаса (Epcos EFD-25/13/9)
//!!!На основе выбранного типа выбираются значения al и a_min
//!!!Сейчас данные значения будут заданы как константы
//!!!Порядок выбора: каркас -> феррит -> зазор (включая ungapped)
//!!!Переменные, относящиеся к детали, можно хранить в виде структуры. ??? НЕ ЗНАЮ
//!!!Здесь также должы запоминатсья кривые удельных потерь феррита (возможно дин. массив коэффициентов уравнения)
//!!!Возможно здесь будет храниться количество выводов  (можно использовать в случае обмотки из нескольких проводов)
//!!!В связи с тем, что параметры могут быть добавлены позже, хранение в файле лучше организовать в виде именованных переменных

	char *coil_former = "Epcos EFD 25/13/9";	//!!!
	char *ferrite = "Epcos N87";	//!!!
	char *gap = "160 нГн";	//!!!

	sprintf(console_line, "Каркас %s", coil_former);
	console_print(console_line);
	sprintf(console_line, "Феррит %s", ferrite);
	console_print(console_line);
	sprintf(console_line, "Зазор %s", gap);
	console_print(console_line);

	//Максимальная рабочая температура каркаса:
	//!!!Определяются типом каркаса
	//!!!Приводятся в DS в явном виде
	//!!!НУЖНО ОБРАБОТАТЬ СРАВНЕНИЕ С ВВЕДЁННОЙ МАКСИМАЛЬНОЙ ТЕМПЕРАТУРОЙ
	//!!!Здесь используются константы
	console_print("Максимальная рабочая температура каркаса:");
	double tcf_max;
	tcf_max = 155;
	sprintf(console_line, "tcf_max = %g°C", tcf_max);
	console_print(console_line);
	//Проверка максимально рабочей температуры каркаса:
	if (tcf_max < t_max)
	{
		//Если максимальная температура окружающей среды превышает максимальную рабочую температуру каркаса:
		console_print("Максимальная температура окружающей среды превышает максимальную рабочую температуру каркаса");
		//!!!Нужно заменить каркас
		//!!!При замене каркаса возможно придётся заменить феррит и зазор
	}

	//Температурный диапазон феррита:
	//!Минимальная рабочая температура феррита:
	//!!!Определяются типом феррита
	//!!!НЕ ПОНЯТНО, КАКУЮ ТЕМПЕРАТУРУ ЗДЕСЬ УКАЗЫВАТЬ!!!
	//!!!ЭТО ВАЖНО, ТАК КАК С УМЕНЬШЕНИЕМ ТЕМПЕРАТУРЫ РАСТУТ ПОТЕРИ В ФЕРРИТЕ!!!
	//!!!Вариатны:
	//!!!1) Использовать минимальную температуру, для которой есть измерения μi на графике Initial permeability versus temperature (-60, хороший вариант)
	//!!!2) Использовать минимальную температуру, для которой есть измерения Pv на графике Relative core losses versus temperature (+25, плохой вариант)
	//!!!СЕЙЧАС ИСПОЛЬЗУЕТСЯ ПЕРВЫЙ ВАРИАНТ
	//!!!НУЖНО ОБРАБОТАТЬ СРАВНЕНИЕ С ВВЕДЁННОЙ МИНИМАЛЬНОЙ ТЕМПЕРАТУРОЙ
	//!!!Здесь используются константы
	//!Максимальная рабочая температура феррита:
	//!!!Определяются типом феррита
	//!!!Используется температура Кюри из DS, других вариантов не видно
	//!!!НУЖНО ОБРАБОТАТЬ СРАВНЕНИЕ С ВВЕДЁННОЙ МАКСИМАЛЬНОЙ ТЕМПЕРАТУРОЙ
	//!!!Здесь используются константы
	console_print("Температурный диапазон феррита:");
	double tcore_min, tcore_max;
	tcore_min = -60;
	sprintf(console_line, "tcore_min = %g°C", tcore_min);
	console_print(console_line);
	tcore_max = 210;
	sprintf(console_line, "tcore_max = %g°C", tcore_max);
	console_print(console_line);
	//Проверка температурного диапазона феррита:
	//!!!ПРИ ПОВТОРНОМ ВЫБОРЕ ФЕРРИТА НУЖНО БУДЕТ УЧИТЫВАТЬ РЕЗУЛЬТАТ ОБОИХ УСЛОВИЙ - УЧЕСТЬ НУЖНО ТАКЖЕ КАРКАС И ПОМЕНЯТЬ СРАЗУ ВСЁ
	if (tcore_min > t_min)
	{
		//Если минимальная температура окружающей среды меньше минимальной рабочей температуры феррита:
		console_print("Минимальная температура окружающей среды меньше минимальной рабочей температуры феррита");
		//!!!Нужно предложить сменить феррит
	}
	if (tcore_max < t_max)
	{
		//Если максимальная температура окружающей среды превышает максимальную рабочую температуру феррита:
		console_print("Максимальная температура окружающей среды превышает максимальную рабочую температуру феррита");
		//!!!Нужно предложить сменить феррит
	}

	//Частотный диапазон феррита:
	//!!!Определяются типом феррита
	//!!!Приводятся в DS на феррит в явном виде
	//!!!НУЖНО ОБРАБОТАТЬ СРАВНЕНИЕ С ВВЕДЁННЫМИ ЧАСТОТАМИ
	//!!!Здесь используются константы.
	console_print("Оптимальный частотный диапазон феррита:");
	double fcore_min, fcore_max;
	fcore_min = 1e+3 * 25;
	sprintf(console_line, "fcore_min = %g кГц", 1e-3 * fcore_min);
	console_print(console_line);
	fcore_max = 1e+3 * 500;
	sprintf(console_line, "fcore_max = %g кГц", 1e-3 * fcore_max);
	console_print(console_line);
	//Проверка частотного диапазона феррита:
	//!!!ПРИ ПОВТОРНОМ ВЫБОРЕ ФЕРРИТА НУЖНО БУДЕТ УЧИТЫВАТЬ РЕЗУЛЬТАТ ОБОИХ УСЛОВИЙ
	if (fcore_min > f_min)
	{
		//Если минимальная частота преобразования не входит в оптимальный частотный диапазон феррита:
		console_print("Минимальная частота преобразования не входит в оптимальный частотный диапазон феррита");
		//!!!Нужно предложить сменить феррит
		//!!!Возможно, стоит предусмотреть возможность использования неподходящих ферритов
	}
	if (fcore_max < f_max)
	{
		//Если максимальная частота преобразования не входит в оптимальный частотный диапазон феррита:
		console_print("Максимальная частота преобразования не входит в оптимальный частотный диапазон феррита");
		//!!!Нужно предложить сменить феррит
		//!!!Возможно, стоит предусмотреть возможность использования неподходящих ферритов
	}

	//Пространство в каркасе:
	//!!!Берётся минимальное
	console_print("Пространство в каркасе:");
	double a;	//!!!ЗАВИСИТ ОТ ВЫБОРА КАРКАСА (ПРИВОДИТСЯ В DS НА ЧЕРТЕЖЕ)
	a = 1e-3 * 16.4;	//!!!Проверил
	sprintf(console_line, "a = %g мм", 1e+3 * a);
	console_print(console_line);

	//Сечение каркаса:
	enum shape
	{
		RECTANGLE,
		ROUND
	};
	enum shape cf_shape;
	cf_shape = RECTANGLE;	//!!!Здесь будет чтение из файла
	double bcf, ccf, dcf;
	switch(cf_shape)
	{
		case RECTANGLE:
			console_print("Сечение каркаса:\nПрямоугольное");
			//Размеры части каркаса, используемой для намотки провода:
			//!!!Определяются типом каркаса
			//!!!Приводятся в DS на чертеже в явном виде
			//!!!Вероятно, нужно будет предусмотреть работу с каркасами круглого сечения
			console_print("Ширина части каркаса, используемой для намотки провода:");
			bcf = 1e-3 * 13.1;	//!!!Проверил
			sprintf(console_line, "bcf = %g мм", 1e+3 * bcf);
			console_print(console_line);
			console_print("Высота части каркаса, используемой для намотки провода:");
			ccf = 1e-3 * 6.9;	//!!!Проверил
			sprintf(console_line, "ccf = %g мм", 1e+3 * ccf);
			console_print(console_line);
			break;
		case ROUND:
			console_print("Сечение каркаса:\nКруглое");
			//Диаметр части каркаса, используемой для намотки провода:
			//!!!Определяются типом каркаса
			//!!!Приводятся в DS на чертеже в явном виде
			//!!!Вероятно, нужно будет предусмотреть работу с каркасами круглого сечения
			console_print("Диаметр части каркаса, используемой для намотки провода:");
			dcf = 1e-3 * 19.3;	//!!!Значение для ETD 49 для проверки круглого режима
			sprintf(console_line, "dcf = %g мм", 1e+3 * dcf);
			console_print(console_line);
			break;
		default:
			//!!!В данном случае нужно выдавать ошибку (если это не будет учтено при чтении файла)
			break;
	}

	//Максимальная толщина обмоток:
	//!!!Определяются типом каркаса
	//!!!Вычисляется на основе чертежа из DS. Берётся минимальное значение
	//!!!Здесь используется константа
	console_print("Максимальная толщина обмоток:");
	double sw_max;
	sw_max = 1e-3 * 2.325;	//!!!Проверил
	sprintf(console_line, "sw_max = %g мм", 1e+3 * sw_max);
	console_print(console_line);

	//Минимальная площадь поперечного сечения сердечника:
	console_print("Минимальная площадь поперечного сечения сердечника:");
	double A_min;	//!!!ЗАВИСИТ ОТ ВЫБОРА КАРКАСА (ПРИВОДИТСЯ В DS)
	A_min = 1e-6 * 57;
	sprintf(console_line, "A_min = %g мм^2", 1e+6 * A_min);
	console_print(console_line);

	//Объём сердечника:
	console_print("Объём сердечника:");
	double Ve;	//!!!ЗАВИСИТ ОТ ВЫБОРА КАРКАСА (ПРИВОДИТСЯ В DS)
	Ve = 1e-9 * 3310;
	sprintf(console_line, "Ve = %g мм^3", 1e+9 * Ve);
	console_print(console_line);

	//Площадь открытой части сердечника:
	//!!!ЗАВИСИТ ОТ КАРКАСА
	//!!!Вычисление описаны на листочке.
	//!!!Размеры из DS берутся РАЗНЫЕ. Нужно будет над этим подумать
	//!!!СЕЙЧАС ИСПОЛЬЗУЕМ ПОЧТИ ВЕРНУЮ КОНСТАНТУ В мм^2
	console_print("Площадь открытой части обмоток:");
	double Score;
	Score = 1e-6 * 1491.4578;
	sprintf(console_line, "Score = %g мм^2", 1e+6 * Score);
	console_print(console_line);

	//Площадь открытой части обмоток:
	//!!!ЗАВИСИТ ОТ КАРКАСА
	//!!!Вычисление описаны на листочке. Предполагается, что обмотки достигают максимальной толщины.
	//!!!Дно не учитывается (проверено). Размеры из DS берутся минимальные
	//!!!СЕЙЧАС ИСПОЛЬЗУЕМ КОНСТАНТУ В мм^2
	console_print("Площадь открытой части сердечника:");
	double Sw;
	Sw = 1e-6 * 678.14;
	sprintf(console_line, "Sw = %g мм^2", 1e+6 * Sw);
	console_print(console_line);

	//Индукция насыщения феррита:
	//!!!Определяется по графикам из DS на феррит
	//!!!Приводится при разных температурах (25°C и 100°C)
	//!!!Нужно будет загружать из файла одно из этих значений
	//!!!Сейчас будем использовать константу (для 100°C)
	//!!!Нужно использовать только одно (худшее) значение,
	//!!!так как B_max сравнивается с ΔB, который от температуры не зависит
	//!!!Худшее значение достигается при температуре 100 град.
	console_print("Индукция насыщения феррита:");
	double B_max;	//!!!ЗАВИСИТ ОТ ВЫБОРА ФЕРРИТА И ОТ ТЕМПЕРАТУРЫ
	B_max = 1e-3 * 380;
	sprintf(console_line, "B_max = %g мТл", 1e+3 * B_max);
	console_print(console_line);

	//Индуктивность на виток в квадрате:
	console_print("Индуктивность на виток в квадрате:");
	double Al;	//!!!ЗАВИСИТ ОТ ВЫБОРА КАРКАСА, ФЕРРИТА И ЗАЗОРА
	Al = 1e-9 * 160;
	sprintf(console_line, "Al = %g нГн", 1e+9 * Al);
	console_print(console_line);

	//Вывод разделителя:
	console_print("- - - - - - - - - -");

	//Число витков первичной обмотки:
	console_print("Число витков первичной обмотки:");
	double N1;
	N1 = sqrt(Lpri / Al);
	sprintf(console_line, "N1 = %g", N1);
	console_print(console_line);
	N1 = floor(N1);
	sprintf(console_line, "N1 = %g", N1);
	console_print(console_line);

	//Индуктивность первичной обмотки (уточнённая):
	console_print("Индуктивность первичной обмотки:");
	Lpri = (pow(N1, 2.) * Al);
	sprintf(console_line, "Lpri = %g мкГн", 1e+6 * Lpri);
	console_print(console_line);

	//Размах индукции в сердечнике при наименее благоприятных условиях:
	console_print("Размах индукции в сердечнике при наименее благоприятных условиях:");
	double DeltaB;
	DeltaB = (Vdc_max * tpri_max) / (A_min * N1);
	sprintf(console_line, "ΔB = %g мТл", 1e+3 * DeltaB);
	console_print(console_line);
	//Проверка размаха индукции:
	if (DeltaB > B_max)
	{
		//Если размах индукции в сердечнике превышает индукцию насыщения сердечника:
		console_print("Размах индукции в сердечнике превышает индукцию насыщения");
		//!!!Нужно увеличить индукцию насыщения за счёт замены феррита
		//!!!Либо увеличить a_min за счёт увеличения размеров каркаса. Замена каркаса может привести к замене феррита и зазора
	}

	//Удельные потери в сердечнике:
	//!!!В DS приводятся кривые для 25°C и 100°C
	//!!!Приведены разные кривые для разных B = ΔB/2
	//!!!Удельные потери зависят от частоты
	//!!!Измеряются в мВт/см^3 = кВт/м^3
	//!!!Нужно вычислять по многомерным кривым, постоенным на основе графиков из DS
	//!!!Для этого логично использовать специальную функцию
	//!!!Форму функции нужно будет читать из файла
	//!!!Сейчас будем использовать неверные константы для 25°C и 100°C
	console_print("Удельные потери в сердечнике при наименее благоприятных условиях:");
	double Pv_25, Pv_100;
	Pv_25 = 1e+3 * 600;
	sprintf(console_line, "Pv(25) = %g кВт/м^3", 1e-3 * Pv_25);
	console_print(console_line);
	Pv_100 = 1e+3 * 400;
	sprintf(console_line, "Pv(100) = %g кВт/м^3", 1e-3 * Pv_100);
	console_print(console_line);

	//Потери в сердечнике:
	console_print("Потери в сердечнике при наименее благоприятных условиях:");
	double Pcore_25, Pcore_100;
	Pcore_25 = Pv_25 * Ve;
	sprintf(console_line, "Pcore(25) = %g мВт", 1e+3 * Pcore_25);
	console_print(console_line);
	Pcore_100 = Pv_100 * Ve;
	sprintf(console_line, "Pcore(100) = %g мВт", 1e+3 * Pcore_100);
	console_print(console_line);

	//Импульсный ток первичной обмотки:
	//Амплитуда пилообразного импульса тока
	console_print("Импульсный ток первичиной обмотки:");
	double Ipri_min, Ipri_nom, Ipri_max;
	Ipri_min = sqrt((2 * Pout_max) / (eta * f_max * Lpri));
	sprintf(console_line, "Ipri_min = %g А", Ipri_min);
	console_print(console_line);
	Ipri_nom = sqrt((2 * Pout_nom) / (eta * f_nom * Lpri));
	sprintf(console_line, "Ipri_nom = %g А", Ipri_nom);
	console_print(console_line);
	Ipri_max = sqrt((2 * Pout_min) / (eta * f_min * Lpri));
	sprintf(console_line, "Ipri_max = %g А", Ipri_max);
	console_print(console_line);

	//Время прямого хода (уточнённое):
	console_print("Время прямого хода:");
	tpri_min = (Ipri_max * Lpri) / Vdc_max;
	sprintf(console_line, "tpri_min = %g мкс", 1e+6 * tpri_min);
	console_print(console_line);
	tpri_nom = (Ipri_nom * Lpri) / Vdc_nom;
	sprintf(console_line, "tpri_nom = %g мкс", 1e+6 * tpri_nom);
	console_print(console_line);
	tpri_max = (Ipri_min * Lpri) / Vdc_min;
	sprintf(console_line, "tpri_max = %g мкс", 1e+6 * tpri_max);
	console_print(console_line);

	//Коэффициент заполнения импульсов тока первичной обмотки:
	console_print("Коэффициент заполнения импульсов тока первичной обмотки:");
	double Dpri_min, Dpri_nom, Dpri_max;
	Dpri_min = tpri_min / T_max;
	sprintf(console_line, "Dpri_min = %g", Dpri_min);
	console_print(console_line);
	Dpri_nom = tpri_nom / T_nom;
	sprintf(console_line, "Dpri_nom = %g", Dpri_nom);
	console_print(console_line);
	Dpri_max = tpri_max / T_min;
	sprintf(console_line, "Dpri_max = %g", Dpri_max);
	console_print(console_line);

	//Среднеквадратичное значение тока первичной обмотки:
	console_print("Среднеквадратичное значение тока первичной обмотки:");
	double Ipri_min_rms, Ipri_nom_rms, Ipri_max_rms;
	Ipri_min_rms = Ipri_max * sqrt(Dpri_min / 3);
	sprintf(console_line, "Ipri_min_rms = %g А", Ipri_min_rms);
	console_print(console_line);
	Ipri_nom_rms = Ipri_nom * sqrt(Dpri_nom / 3);
	sprintf(console_line, "Ipri_nom_rms = %g А", Ipri_nom_rms);
	console_print(console_line);
	Ipri_max_rms = Ipri_min * sqrt(Dpri_max / 3);
	sprintf(console_line, "Ipri_max_rms = %g А", Ipri_max_rms);
	console_print(console_line);

	//Число витков вторичной обмотки:
	console_print("Число витков вторичной обмотки:");
	double N2;
	N2 = sqrt(Lsec / Al);
	sprintf(console_line, "N2 = %g", N2);
	console_print(console_line);
	N2 = floor(N2);
	sprintf(console_line, "N2 = %g", N2);
	console_print(console_line);

	//Индуктивность вторичной обмотки (уточнённая):
	console_print("Индуктивность вторичной обмотки:");
	Lsec = (pow(N2, 2.) * Al);
	sprintf(console_line, "Lsec = %g мкГн", 1e+6 * Lsec);
	console_print(console_line);

	//Коэффициент трансформации:
	console_print("Коэффициент трансформации:");
	double K;
	K = sqrt(Lpri / Lsec);
	sprintf(console_line, "K = %g", K);
	console_print(console_line);

	//Максимальное напряжение на силовом ключе (без учёта индуктивного выброса):
	console_print("Максимальное напряжение на силовом ключе:");
	double Vds;
	Vds = Vdc_max + (Vout_max + Vdout) * K;
	sprintf(console_line, "Vds = %g В", Vds);
	console_print(console_line);

	//Импульсный ток вторичной обмотки:
	//Амплитуда пилообразного импульса тока
	console_print("Импульсный ток вторичной обмотки:");
	double Isec_min, Isec_nom, Isec_max;
	Isec_min = Ipri_min * K;
	sprintf(console_line, "Isec_min = %g А", Isec_min);
	console_print(console_line);
	Isec_nom = Ipri_nom * K;
	sprintf(console_line, "Isec_nom = %g А", Isec_nom);
	console_print(console_line);
	Isec_max = Ipri_max * K;
	sprintf(console_line, "Isec_max = %g А", Isec_max);
	console_print(console_line);

	//Время обратного хода (уточнённое):
	console_print("Время обратного хода:");
	tsec_min = (Isec_min * Lsec) / (Vout_max + Vdout);
	sprintf(console_line, "tsec_min = %g мкс", 1e+6 * tsec_min);
	console_print(console_line);
	tsec_nom = (Isec_nom * Lsec) / (Vout_nom + Vdout);
	sprintf(console_line, "tsec_nom = %g мкс", 1e+6 * tsec_nom);
	console_print(console_line);
	tsec_max = (Isec_max * Lsec) / (Vout_min + Vdout);
	sprintf(console_line, "tsec_max = %g мкс", 1e+6 * tsec_max);
	console_print(console_line);

	//Коэффициент заполнения импульсов тока вторичной обмотки:
	console_print("Коэффициент заполнения импульсов тока вторичной обмотки:");
	double Dsec_min, Dsec_nom, Dsec_max;
	Dsec_min = tsec_max / T_max;
	sprintf(console_line, "Dsec_min = %g", Dsec_min);
	console_print(console_line);
	Dsec_nom = tsec_nom / T_nom;
	sprintf(console_line, "Dsec_nom = %g", Dsec_nom);
	console_print(console_line);
	Dsec_max = tsec_min / T_min;	//Случай1
	sprintf(console_line, "Dsec_max = %g", Dsec_max);
	console_print(console_line);

	//Среднеквадратичное значение тока вторичной обмотки:
	console_print("Среднеквадратичное значение тока вторичной обмотки:");
	double Isec_min_rms, Isec_nom_rms, Isec_max_rms;
	Isec_min_rms = Isec_min * sqrt(Dsec_max / 3);
	sprintf(console_line, "Isec_min_rms = %g А", Isec_min_rms);
	console_print(console_line);
	Isec_nom_rms = Isec_nom * sqrt(Dsec_nom / 3);
	sprintf(console_line, "Isec_nom_rms = %g А", Isec_nom_rms);
	console_print(console_line);
	Isec_max_rms = Isec_max * sqrt(Dsec_min / 3);
	sprintf(console_line, "Isec_max_rms = %g А", Isec_max_rms);
	console_print(console_line);

	//Постоянная составляющая тока вторичной обмотки:
	console_print("Постоянная составляющая тока вторичной обмотки:");
	double Isec_min_dc, Isec_nom_dc, Isec_max_dc;
	Isec_min_dc = Isec_max * (Dsec_min / 2);
	sprintf(console_line, "Isec_min_dc = %g А", Isec_min_dc);
	console_print(console_line);
	Isec_nom_dc = Isec_nom * (Dsec_nom / 2);
	sprintf(console_line, "Isec_nom_dc = %g А", Isec_nom_dc);
	console_print(console_line);
	Isec_max_dc = Isec_min * (Dsec_max / 2);
	sprintf(console_line, "Isec_max_dc = %g А", Isec_max_dc);
	console_print(console_line);

	//Переменная составляющая тока вторичной обмотки:
	console_print("Переменная составляющая тока вторичной обмотки:");
	double Isec_min_ac, Isec_nom_ac, Isec_max_ac;
	Isec_min_ac = Isec_min * sqrt((Dsec_max / 3) - (pow(Dsec_max, 2.) / 4));
	sprintf(console_line, "Isec_min_ac = %g А", Isec_min_ac);
	console_print(console_line);
	Isec_nom_ac = Isec_nom * sqrt((Dsec_nom / 3) - (pow(Dsec_nom, 2.) / 4));
	sprintf(console_line, "Isec_nom_ac = %g А", Isec_nom_ac);
	console_print(console_line);
	Isec_max_ac = Isec_max * sqrt((Dsec_min / 3) - (pow(Dsec_min, 2.) / 4));
	sprintf(console_line, "Isec_max_ac = %g А", Isec_max_ac);
	console_print(console_line);
/*

	//Напряжение питания ШИМ-контроллера:
	//!!!ВЫБОР КОНТРОЛЛЕРА МОЖНО ПОМЕСТИТЬ В НАЧАЛО (в виде виджета), ТАК КАК ОТ ЭТОГО ЗАВИСИТ РАБОЧИЙ ЦИКЛ
	//!!!Будем использовать константы для ON Semiconductor UC3844B, BV
	double Vbias_min, Vbias_nom, Vbias_max;	//!!!ЗАВИСИТ ОТ ВЫБОРА КОНТРОЛЛЕРА
	Vbias_min = 11.5;	//!!!Minimum Operating Voltage After Turn−On (max)
	Vbias_max = 16.0;	//!!!Startup Threshold (typ)
	Vbias_nom = (Vbias_min + Vbias_max) / 2;
	sprintf(console_line, "Vbias_min = %g В", Vbias_min);
	console_print(console_line);
	sprintf(console_line, "Vbias_nom = %g В", Vbias_nom);
	console_print(console_line);
	sprintf(console_line, "Vbias_max = %g В", Vbias_max);
	console_print(console_line);

	//Прямое падение напряжения на диоде в цепи питания ШИМ-контроллера:
	double Vd;
	Vd = 0.6;
	sprintf(console_line, "Vd = %g В", Vd);
	console_print(console_line);

	//Число витков обмотки питания ШИМ-контроллера:
	double Nbias;
	Nbias = ((Vbias_nom + Vd) * N2) / (Vout_nom + Vdout);
	sprintf(console_line, "Nbias = %g", Nbias);
	console_print(console_line);
	Nbias = floor(Nbias + 0.5);
	sprintf(console_line, "Nbias = %g", Nbias);
	console_print(console_line);

	//Напряжение питания ШИМ-контроллера (уточнённое):
	Vbias_nom = ((Nbias / N2) * (Vout_nom + Vdout)) - Vd;
	sprintf(console_line, "Vbias_nom = %g В", Vbias_nom);
	console_print(console_line);

	//Проверка напряжения питания ШИМ-контроллера:
	if (Vbias_nom < Vbias_min)
	{
		//Если напряжение обмотки питания меньше минимально допустимого:
		console_print("Напряжение обмотки питания ШИМ-контроллера меньше минимально допустимого");
		//!!!ЗДЕСЬ НУЖНО АВТОМАТИЧЕСКИ УВЕЛИЧИВАТЬ КОЛИЧЕСТВО ВИТКОВ
	} else if (Vbias_nom > Vbias_max) {
		//Если напряжение обмотки питания больше максимально допустимого:
		console_print("Напряжение обмотки питания ШИМ-контроллера больше максимально допустимого");
		//!!!ЗДЕСЬ НУЖНО АВТОМАТИЧЕСКИ УМЕНЬШАТЬ КОЛИЧЕСТВО ВИТКОВ
	}

//!!!НУЖНО ПРЕДУСМОТРЕТЬ ВОЗМОЖНОСТЬ СМЕНЫ ШИМ-КОНТРОЛЛЕРА ИЛИ ИЗМЕНЕНИЯ ДРУГИХ ОБМОТОК ПРИ ОТСУТСТВИИ ВОЗМОЖНОСТИ ПОПАСТЬ В ДИАПАЗОН

//!!!НАЧИНАЕТСЯ КОНСТРУКТОРСКАЯ ЧАСТЬ:

	//Максимальный диаметр провода первичной обмотки:
	double d1_max;
	d1_max = a / N1;
	sprintf(console_line, "d1_max = %g мм", 1e+3 * d1_max);
	console_print(console_line);

	//Выбор провода первичной обмотки:
	//!!!ЗДЕСЬ НУЖНО ОРГАНИЗОВАТЬ ЧТЕНИЕ ФАЙЛОВ, ОПИСЫВАЮЩИХ ПРОВОДА, И ВЫБОР МАРКИ ПРОВОДА
	//!!!При расчёте геометрии лучше использовать МАКСИМАЛЬНЫЕ диаметры
	//!!!Сейчас используется константа, соответствующая номинальному диаметру
	//!!!В УЧЕБНИКЕ ДЛЯ ПЭТВ-2 ПРИВОДЯТСЯ РАЗМЕРЫ ПО МЕДИ И РАЗМЕРЫ ПО ИЗОЛЯЦИИ.
	//!!!НУЖНО БУДЕТ ПЕРЕДЕЛАТЬ ВСЁ ТАК, ЧТОБЫ ПРИ РАСЧЁТЕ ГЕОМЕТРИИ ИСПОЛЬЗОВАЛСЯ ДИАМЕТР ПО ИЗОЛЯЦИИ
	//!!!А ПРИ РАСЧЁТЕ СОПРОТИВЛЕНИЙ ИСПОЛЬЗОВАЛСЯ ДИАМЕТР ПО МЕДИ
	double d1_min, d1_nom;
	d1_min = 1e-3 * 0.18;	//!!!Минимальный
	sprintf(console_line, "d1_min = %g мм", 1e+3 * d1_min);
	console_print(console_line);
	d1_nom = 1e-3 * 0.18;	//!!!Номинальный диаметр
	sprintf(console_line, "d1_nom = %g мм", 1e+3 * d1_nom);
	console_print(console_line);
	d1_max = 1e-3 * 0.18;	//!!!Максимальный диаметр
	sprintf(console_line, "d1_max = %g мм", 1e+3 * d1_max);
	console_print(console_line);

	//Удельное сопротивление меди:
	double rhocu_25, rhocu_100;
	rhocu_25 = 1e-6 * 0.0172;
	sprintf(console_line, "ρcu(25) = %g Ом*мм^2/м", 1e+6 * rhocu_25);
	console_print(console_line);
	rhocu_100 = 1e-6 * 0.0226;
	sprintf(console_line, "ρcu(100) = %g Ом*мм^2/м", 1e+6 * rhocu_100);
	console_print(console_line);

	//Число π:
	const double pi = acos(-1);

	//Сопротивление одного метра провода первичной обмотки выбранного диаметра:
	//При вычислении сопротивлений необходимо использовать минимальный диаметр
	double Rpri_25_m, Rpri_100_m;
	Rpri_25_m = rhocu_25 * (4 / (pi * pow(d1_min, 2.)));
	sprintf(console_line, "Rpri(25)m = %g Ом/м", Rpri_25_m);
	console_print(console_line);
	Rpri_100_m = rhocu_100 * (4 / (pi * pow(d1_min, 2.)));
	sprintf(console_line, "Rpri(100)m = %g Ом/м", Rpri_100_m);
	console_print(console_line);

	//Длина витка первичной обмотки:
	//При вычислении длины витка необходимо использовать максимальный диаметр
	double lpri;
	lpri = 2 * (b + c + 2 * d1_max);
	sprintf(console_line, "lpri = %g мм", 1e+3 * lpri);
	console_print(console_line);

	//Сопротивление первичной обмотки (без учёта эффекта близости):
	double Rpri_25, Rpri_100;
	Rpri_25 = N1 * Rpri_25_m * lpri;
	sprintf(console_line, "Rpri(25) = %g Ом", Rpri_25);
	console_print(console_line);
	Rpri_100 = N1 * Rpri_100_m * lpri;
	sprintf(console_line, "Rpri(100) = %g Ом", Rpri_100);
	console_print(console_line);

	//Потери мощности в первичной обмотке:
	double Ppri_25, Ppri_100;
	Ppri_25 = pow(Ipri_nom_rms, 2.) * Rpri_25;
	sprintf(console_line, "Ppri(25) = %g мВт", 1e+3 * Ppri_25);
	console_print(console_line);
	Ppri_100 = pow(Ipri_nom_rms, 2.) * Rpri_100;
	sprintf(console_line, "Ppri(100) = %g мВт", 1e+3 * Ppri_100);
	console_print(console_line);

	//Выбор провода обмотки питания:
	//!!!Провод обмотки питания будет совпадать с проводом первички
	//!!!ЗДЕСЬ ВОЗМОЖНО ПРИДЁТСЯ ЗАПИСЫВАТЬ НАЗВАНИЕ ПРОВОДА В КАКУЮ-ТО ПЕРЕМЕННУЮ
	double dbias_max;
	dbias_max = d1_max;
	sprintf(console_line, "dbias_max = %g мм", 1e+3 * dbias_max);
	console_print(console_line);

	//Максимальный диаметр провода вторичной обмотки:
	double d2_max;
	d2_max = a / N2;
	sprintf(console_line, "d2_max = %g мм", 1e+3 * d2_max);
	console_print(console_line);

	//Выбор провода вторичной обмотки:
	//!!!ЗДЕСЬ НУЖНО ОРГАНИЗОВАТЬ ЧТЕНИЕ ФАЙЛОВ, ОПИСЫВАЮЩИХ ПРОВОДА, И ВЫБОР МАРКИ ПРОВОДА
	//!!!При расчёте геометрии лучше использовать МАКСИМАЛЬНЫЕ диаметры
	//!!!Сейчас используется константа, соответствующая номинальному диаметру
	//!!!В УЧЕБНИКЕ ДЛЯ ПЭТВ-2 ПРИВОДЯТСЯ РАЗМЕРЫ ПО МЕДИ И РАЗМЕРЫ ПО ИЗОЛЯЦИИ.
	//!!!НУЖНО БУДЕТ ПЕРЕДЕЛАТЬ ВСЁ ТАК, ЧТОБЫ ПРИ РАСЧЁТЕ ГЕОМЕТРИИ ИСПОЛЬЗОВАЛСЯ ДИАМЕТР ПО ИЗОЛЯЦИИ
	//!!!А ПРИ РАСЧЁТЕ СОПРОТИВЛЕНИЙ ИСПОЛЬЗОВАЛСЯ ДИАМЕТР ПО МЕДИ
	double d2_min, d2_nom;
	d2_min = 1e-3 * 2.5;	//!!!Минимальный
	sprintf(console_line, "d2_min = %g мм", 1e+3 * d2_min);
	console_print(console_line);
	d2_nom = 1e-3 * 2.5;	//!!!Номинальный диаметр
	sprintf(console_line, "d2_nom = %g мм", 1e+3 * d2_nom);
	console_print(console_line);
	d2_max = 1e-3 * 2.5;	//!!!Максимальный диаметр
	sprintf(console_line, "d2_max = %g мм", 1e+3 * d2_max);
	console_print(console_line);

//!!!С ЭТОГО МОМЕНТА МОЖНО ПРОВЕРЯТЬ, ВЛЕЗУТ ЛИ ОБМОТКИ В КАРКАС!!!

	//Сопротивление одного метра провода вторичной обмотки выбранного диаметра:
	//При вычислении сопротивлений необходимо использовать минимальный диаметр
	double Rsec_25_m, Rsec_100_m;
	Rsec_25_m = rhocu_25 * (4 / (pi * pow(d2_min, 2.)));
	sprintf(console_line, "Rsec(25)m = %g мОм/м", 1e+3 * Rsec_25_m);
	console_print(console_line);
	Rsec_100_m = rhocu_100 * (4 / (pi * pow(d2_min, 2.)));
	sprintf(console_line, "Rsec(100)m = %g мОм/м", 1e+3 * Rsec_100_m);
	console_print(console_line);

	//Длина витка вторичной обмотки:
	//При вычислении длины витка необходимо использовать максимальный диаметр
	double lsec;
	lsec = 2 * (b + c + 8 * d1_max + 2 * d2_max);
	sprintf(console_line, "lsec = %g мм", 1e+3 * lsec);
	console_print(console_line);

	//Сопротивление вторичной обмотки постоянному току:
	double Rsec_25_dc, Rsec_100_dc;
	Rsec_25_dc = N2 * Rsec_25_m * lsec;
	sprintf(console_line, "Rsec(25)dc = %g мОм/м", 1e+3 * Rsec_25_dc);
	console_print(console_line);
	Rsec_100_dc = N2 * Rsec_100_m * lsec;
	sprintf(console_line, "Rsec(100)dc = %g мОм/м", 1e+3 * Rsec_100_dc);
	console_print(console_line);

	//Глубина проникновения ВЧ тока в проводник:
	//!!!Можно усложнить вычисление, использовав удельное сопротивление меди
	double Dpen;
	Dpen = 0.075 / sqrt(f_nom);
	sprintf(console_line, "Dpen = %g мм", 1e+3 * Dpen);
	console_print(console_line);

	//Эффективная толщина слоя:
	//!!!Можно учесть влияние на LTH неплотной упаковки проводников
	//!!!Возможно лучше использовать минимальный или максимальный диаметр
	double LTH;
	LTH = 0.83 * d2_nom;
	sprintf(console_line, "LTH = %g мм", 1e+3 * LTH);
	console_print(console_line);

	//Отношение эффективной толщины слоя и глубины проникновения ВЧ тока в проводник:
	double Q;
	Q = LTH / Dpen;
	sprintf(console_line, "Q = %g", Q);
	console_print(console_line);

	//Отношение сопротивления переменному току и сопротивления постоянному току:
	//!!!НЕОБХОДИМО ВЫЧИСЛЯТЬ НА ОСНОВЕ КРИВОЙ ДАУЭЛЛА ДЛЯ ОДНОГО СЛОЯ (2-АЯ ПО СЧЁТУ)
	//!!!Сейчас используется совершенно неправильная константа из диплома
	double Fr;
	Fr = 1;
	sprintf(console_line, "Fr = %g", Fr);
	console_print(console_line);

	//Сопротивление вторичной обмотки переменному току:
	double Rsec_25_ac, Rsec_100_ac;
	Rsec_25_ac = Fr * Rsec_25_dc;
	sprintf(console_line, "Rsec(25)ac = %g мОм/м", 1e+3 * Rsec_25_ac);
	console_print(console_line);
	Rsec_100_ac = Fr * Rsec_100_dc;
	sprintf(console_line, "Rsec(100)ac = %g мОм/м", 1e+3 * Rsec_100_ac);
	console_print(console_line);

	//Потери мощности во вторичной обмотке:
	double Psec_25, Psec_100;
	Psec_25 = pow(Isec_nom_dc, 2.) * Rsec_25_dc + pow(Isec_nom_ac, 2.) * Rsec_25_ac;
	sprintf(console_line, "Psec(25) = %g мВт", 1e+3 * Psec_25);
	console_print(console_line);
	Psec_100 = pow(Isec_nom_dc, 2.) * Rsec_100_dc + pow(Isec_nom_ac, 2.) * Rsec_100_ac;
	sprintf(console_line, "Psec(100) = %g мВт", 1e+3 * Psec_100);
	console_print(console_line);

	//Суммарные потери мощности в трансформаторе:
	double Pt_25, Pt_100;
	Pt_25 = Pcore_25 + Ppri_25 + Psec_25;
	sprintf(console_line, "Pt(25) = %g мВт", 1e+3 * Pt_25);
	console_print(console_line);
	Pt_100 = Pcore_100 + Ppri_100 + Psec_100;;
	sprintf(console_line, "Pt(100) = %g мВт", 1e+3 * Pt_100);
	console_print(console_line);

	//Перегрев сердечника относительно окружающей среды:
	double Deltatcore;
	Deltatcore = pow(1e-1 * (Pcore_25 / Score), 0.833);
	sprintf(console_line, "Δtcore = %g°C", Deltatcore);
	console_print(console_line);

	//Перегрев обмоток относительно окружающей среды:
	//!!!Здесь, возможно, лучше использовать значение для 100 град. Нужно подумать
	double Deltatw;
	Deltatw = pow(1e-1 * ((Ppri_25 + Psec_25) / Sw), 0.833);
	sprintf(console_line, "Δtw = %g°C", Deltatw);
	console_print(console_line);*/

//!!!ЗДЕСЬ НУЖНО ОЦЕНИТЬ МАКСИМАЛЬНЫЕ ТЕМПЕРАТУРЫ ВСЕХ ЭЛЕМЕНТОВ ТРАНСФОРМАТОРА
//!!!ЛЕНТУ НАВЕРНОЕ ГРЕЮТ ТОЛЬКО ОБМОТКИ
//!!!ВЫБРАТЬ САМЫЙ НЕУСТОЙЧИВЫЙ, УЗНАТЬ ЕГО МАКСИМАЛЬНУЮ ТЕМПЕРАТУРУ
//!!!ВЫЧИСЛИТЬ ДОПУСТИМУЮ ТЕМПЕРАТУРУ ОКР СРЕДЫ
//!!!ПРОВЕРИТЬ СООТВЕТСТВИЕ ТРЕБОВАНИЯМ ПО ТЕМПЕРАТУРЕ
}

int main(int argc, char *argv[])
{
	gtk_init(&argc, &argv);
	GtkWidget *window;
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(window), 0, 0);
	gtk_window_set_title(GTK_WINDOW(window), "Расчёт трансформатора");
	gtk_container_set_border_width(GTK_CONTAINER(window), 10);
	g_signal_connect(window, "destroy", G_CALLBACK(window_destroy_cb), NULL);	//(36)!!!заменил на destroy

	//Основной вертикальный контейнер vbox_main:
	GtkWidget *vbox_main;
	vbox_main = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	gtk_container_add(GTK_CONTAINER(window), vbox_main);

	//Меню menu_bar_main:
	GtkWidget *menu_bar_main, *menu_item_main, *menu_main_file;
	menu_bar_main = gtk_menu_bar_new();
	menu_main_file = gtk_menu_new();
	//Субменю "Файл":
	menu_item_main = gtk_menu_item_new_with_mnemonic("_Файл");
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item_main), menu_main_file);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar_main), menu_item_main);
	menu_item_main = gtk_menu_item_new_with_mnemonic("_Открыть");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_main_file), menu_item_main);
	g_signal_connect(menu_item_main, "activate", G_CALLBACK(menu_main_response), "Open");
	menu_item_main = gtk_menu_item_new_with_mnemonic("_Сохранить");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_main_file), menu_item_main);
	g_signal_connect(menu_item_main, "activate", G_CALLBACK(menu_main_response), "Save");
	gtk_box_pack_start(GTK_BOX(vbox_main), menu_bar_main, FALSE, FALSE, 0);
/*
	//Виджеты для ввода исходных данных:
	GtkWidget *vbox_input, *hbox_input, *label_input;
	//Виджеты entry объявлены как глобальные переменые
	//Это позволяет обращаться к их текстам из других функций
	hbox_input = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	//Столбец label'ов:
	vbox_input = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	label_input = gtk_label_new("Параметр ИВЭП");
	gtk_box_pack_start(GTK_BOX(vbox_input), label_input, FALSE, FALSE, 0);
	label_input = gtk_label_new("Выходное напряжение, В");
	gtk_box_pack_start(GTK_BOX(vbox_input), label_input, FALSE, FALSE, 6);
	label_input = gtk_label_new("Ток нагрузки, А");
	gtk_box_pack_start(GTK_BOX(vbox_input), label_input, FALSE, FALSE, 6);
	label_input = gtk_label_new("КПД");
	gtk_box_pack_start(GTK_BOX(vbox_input), label_input, FALSE, FALSE, 6);
	label_input = gtk_label_new("Частота преобразования, кГц");
	gtk_box_pack_start(GTK_BOX(vbox_input), label_input, FALSE, FALSE, 6);
	label_input = gtk_label_new("Рабочий цикл");
	gtk_box_pack_start(GTK_BOX(vbox_input), label_input, FALSE, FALSE, 6);
	label_input = gtk_label_new("Температура окр. среды, °C");
	gtk_box_pack_start(GTK_BOX(vbox_input), label_input, FALSE, FALSE, 6);
	label_input = gtk_label_new("Действ. значение входного напряжения, В");
	gtk_box_pack_start(GTK_BOX(vbox_input), label_input, FALSE, FALSE, 6);
	gtk_box_pack_start(GTK_BOX(hbox_input), vbox_input, FALSE, FALSE, 6);
	//Столбец entry для минимальных значений:
	vbox_input = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	label_input = gtk_label_new("МИН");
	gtk_box_pack_start(GTK_BOX(vbox_input), label_input, FALSE, FALSE, 0);
	entry_input_vout_min = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry_input_vout_min), 5);
	gtk_box_pack_start(GTK_BOX(vbox_input), entry_input_vout_min, FALSE, FALSE, 0);
	entry_input_iout_min = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry_input_iout_min), 5);
	gtk_editable_set_editable(GTK_EDITABLE(entry_input_iout_min), FALSE);	//NEW
	gtk_box_pack_start(GTK_BOX(vbox_input), entry_input_iout_min, FALSE, FALSE, 0);
	entry_input_eta_min = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry_input_eta_min), 5);
	gtk_editable_set_editable(GTK_EDITABLE(entry_input_eta_min), FALSE);	//NEW
	gtk_box_pack_start(GTK_BOX(vbox_input), entry_input_eta_min, FALSE, FALSE, 0);
	entry_input_f_min = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry_input_f_min), 5);
	gtk_box_pack_start(GTK_BOX(vbox_input), entry_input_f_min, FALSE, FALSE, 0);
	entry_input_d_min = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry_input_d_min), 5);
	gtk_box_pack_start(GTK_BOX(vbox_input), entry_input_d_min, FALSE, FALSE, 0);
	entry_input_t_min = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry_input_t_min), 5);
	gtk_box_pack_start(GTK_BOX(vbox_input), entry_input_t_min, FALSE, FALSE, 0);
	entry_input_vin_min = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry_input_vin_min), 5);
	gtk_box_pack_start(GTK_BOX(vbox_input), entry_input_vin_min, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox_input), vbox_input, TRUE, TRUE, 0);
	//Столбец entry для номинальных значений:
	vbox_input = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	label_input = gtk_label_new("НОМ");
	gtk_box_pack_start(GTK_BOX(vbox_input), label_input, FALSE, FALSE, 0);
	entry_input_vout_nom = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry_input_vout_nom), 5);
	gtk_box_pack_start(GTK_BOX(vbox_input), entry_input_vout_nom, FALSE, FALSE, 0);
	entry_input_iout_nom = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry_input_iout_nom), 5);
	gtk_editable_set_editable(GTK_EDITABLE(entry_input_iout_nom), FALSE);	//NEW
	gtk_box_pack_start(GTK_BOX(vbox_input), entry_input_iout_nom, FALSE, FALSE, 0);
	entry_input_eta_nom = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry_input_eta_nom), 5);
	gtk_box_pack_start(GTK_BOX(vbox_input), entry_input_eta_nom, FALSE, FALSE, 0);
	entry_input_f_nom = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry_input_f_nom), 5);
	gtk_box_pack_start(GTK_BOX(vbox_input), entry_input_f_nom, FALSE, FALSE, 0);
	entry_input_d_nom = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry_input_d_nom), 5);
	gtk_editable_set_editable(GTK_EDITABLE(entry_input_d_nom), FALSE);	//NEW
	gtk_box_pack_start(GTK_BOX(vbox_input), entry_input_d_nom, FALSE, FALSE, 0);
	entry_input_t_nom = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry_input_t_nom), 5);
	gtk_editable_set_editable(GTK_EDITABLE(entry_input_t_nom), FALSE);	//NEW
	gtk_box_pack_start(GTK_BOX(vbox_input), entry_input_t_nom, FALSE, FALSE, 0);
	entry_input_vin_nom = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry_input_vin_nom), 5);
	gtk_box_pack_start(GTK_BOX(vbox_input), entry_input_vin_nom, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox_input), vbox_input, TRUE, TRUE, 0);
	//Столбец entry для максимальных значений:
	vbox_input = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	label_input = gtk_label_new("МАКС");
	gtk_box_pack_start(GTK_BOX(vbox_input), label_input, FALSE, FALSE, 0);
	entry_input_vout_max = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry_input_vout_max), 5);
	gtk_box_pack_start(GTK_BOX(vbox_input), entry_input_vout_max, FALSE, FALSE, 0);
	entry_input_iout_max = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry_input_iout_max), 5);
	gtk_box_pack_start(GTK_BOX(vbox_input), entry_input_iout_max, FALSE, FALSE, 0);
	entry_input_eta_max = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry_input_eta_max), 5);
	gtk_editable_set_editable(GTK_EDITABLE(entry_input_eta_max), FALSE);	//NEW
	gtk_box_pack_start(GTK_BOX(vbox_input), entry_input_eta_max, FALSE, FALSE, 0);
	entry_input_f_max = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry_input_f_max), 5);
	gtk_box_pack_start(GTK_BOX(vbox_input), entry_input_f_max, FALSE, FALSE, 0);
	entry_input_d_max = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry_input_d_max), 5);
	gtk_box_pack_start(GTK_BOX(vbox_input), entry_input_d_max, FALSE, FALSE, 0);
	entry_input_t_max = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry_input_t_max), 5);
	gtk_box_pack_start(GTK_BOX(vbox_input), entry_input_t_max, FALSE, FALSE, 0);
	entry_input_vin_max = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry_input_vin_max), 5);
	gtk_box_pack_start(GTK_BOX(vbox_input), entry_input_vin_max, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox_input), vbox_input, TRUE, TRUE, 0);
	//Столбец entry для значений допусков:
	vbox_input = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	label_input = gtk_label_new("ДОП, %");
	gtk_box_pack_start(GTK_BOX(vbox_input), label_input, FALSE, FALSE, 0);
	entry_input_vout_tol = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry_input_vout_tol), 5);
	gtk_box_pack_start(GTK_BOX(vbox_input), entry_input_vout_tol, FALSE, FALSE, 0);
	entry_input_iout_tol = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry_input_iout_tol), 5);
	gtk_editable_set_editable(GTK_EDITABLE(entry_input_iout_tol), FALSE);	//NEW
	gtk_box_pack_start(GTK_BOX(vbox_input), entry_input_iout_tol, FALSE, FALSE, 0);
	entry_input_eta_tol = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry_input_eta_tol), 5);
	gtk_editable_set_editable(GTK_EDITABLE(entry_input_eta_tol), FALSE);	//NEW
	gtk_box_pack_start(GTK_BOX(vbox_input), entry_input_eta_tol, FALSE, FALSE, 0);
	entry_input_f_tol = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry_input_f_tol), 5);
	gtk_box_pack_start(GTK_BOX(vbox_input), entry_input_f_tol, FALSE, FALSE, 0);
	entry_input_d_tol = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry_input_d_tol), 5);
	gtk_editable_set_editable(GTK_EDITABLE(entry_input_d_tol), FALSE);	//NEW
	gtk_box_pack_start(GTK_BOX(vbox_input), entry_input_d_tol, FALSE, FALSE, 0);
	entry_input_t_tol = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry_input_t_tol), 5);
	gtk_editable_set_editable(GTK_EDITABLE(entry_input_t_tol), FALSE);	//NEW
	gtk_box_pack_start(GTK_BOX(vbox_input), entry_input_t_tol, FALSE, FALSE, 0);
	entry_input_vin_tol = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry_input_vin_tol), 5);
	gtk_box_pack_start(GTK_BOX(vbox_input), entry_input_vin_tol, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox_input), vbox_input, TRUE, TRUE, 0);
	//Горизонтальный контейнер, объединяющий все столбцы:
	gtk_box_pack_start(GTK_BOX(vbox_main), hbox_input, FALSE, FALSE, 0);

	//Кнопка начала расчёта button_start:
	GtkWidget *button_start;
	button_start = gtk_button_new_with_label("Начать расчёт");
	g_signal_connect(button_start, "clicked", G_CALLBACK(start_calculation), NULL);
	gtk_box_pack_start(GTK_BOX(vbox_main), button_start, FALSE, FALSE, 0);

	//Прокручиваемый контейнер scrolled_window_console для консоли:
	GtkWidget *scrolled_window_console;
	scrolled_window_console = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scrolled_window_console), 102);
	gtk_box_pack_start(GTK_BOX(vbox_main), scrolled_window_console, TRUE, TRUE, 0);

	//Консоль text_view_console, её буфер, итератор и метка:
	//Виджет, буфер, итератор и метка объявлены как глобальные переменные
	//Это позволяет обращаться к ним из функции console_print
	text_view_console = gtk_text_view_new();
	gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view_console), FALSE);
	text_buffer_console = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view_console));
	gtk_text_buffer_get_iter_at_offset(text_buffer_console, &text_iter_console, 0);
	gtk_container_add(GTK_CONTAINER(scrolled_window_console), text_view_console);
*/
	//Рамка frame1 для области рисования drawing_area_graph1:(36)
	GtkWidget *frame1;
	frame1 = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame1), GTK_SHADOW_IN);
	gtk_box_pack_start(GTK_BOX(vbox_main), frame1, TRUE, TRUE, 0);

	//Область рисования drawing_area_graph1:(36)
	//GtkWidget *drawing_area_graph1;
	drawing_area_graph1 = gtk_drawing_area_new();
	gtk_container_add(GTK_CONTAINER(frame1), drawing_area_graph1);

	//Рамка frame2 для области рисования drawing_area_graph2:(36)
	GtkWidget *frame2;
	frame2 = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame2), GTK_SHADOW_IN);
	gtk_box_pack_start(GTK_BOX(vbox_main), frame2, TRUE, TRUE, 0);

	//Область рисования drawing_area_graph2:(36)
	//GtkWidget *drawing_area_graph2;
	drawing_area_graph2 = gtk_drawing_area_new();
	gtk_container_add(GTK_CONTAINER(frame2), drawing_area_graph2);

	//Рамка frame3 для области рисования drawing_area_graph3:(36)
	GtkWidget *frame3;
	frame3 = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame3), GTK_SHADOW_IN);
	gtk_box_pack_start(GTK_BOX(vbox_main), frame3, TRUE, TRUE, 0);

	//Область рисования drawing_area_graph3:(36)
	//GtkWidget *drawing_area_graph3;
	drawing_area_graph3 = gtk_drawing_area_new();
	gtk_container_add(GTK_CONTAINER(frame3), drawing_area_graph3);

struct graph_appearance default_graph_appearance;	//(36)
default_graph_appearance.background_color.r = 0.5;
default_graph_appearance.background_color.g = 0.5;
default_graph_appearance.background_color.b = 0.0;
default_graph_appearance.major_gridlines_thickness = 1.0;
default_graph_appearance.major_gridlines_color.r = 0.0;
default_graph_appearance.major_gridlines_color.g = 0.0;
default_graph_appearance.major_gridlines_color.b = 0.0;
default_graph_appearance.minor_gridlines_thickness = 1.0;
default_graph_appearance.minor_gridlines_color.r = 0.0;
default_graph_appearance.minor_gridlines_color.g = 0.0;
default_graph_appearance.minor_gridlines_color.b = 1.0;
default_graph_appearance.tick_labels_font.name = "Liberation Serif";
default_graph_appearance.tick_labels_font.size = 12.0;
default_graph_appearance.tick_labels_font.slant = CAIRO_FONT_SLANT_NORMAL;
default_graph_appearance.tick_labels_font.weight = CAIRO_FONT_WEIGHT_NORMAL;
default_graph_appearance.tick_labels_color.r = 0.0;
default_graph_appearance.tick_labels_color.g = 0.3;
default_graph_appearance.tick_labels_color.b = 0.0;
default_graph_appearance.axis_labels_font.name = "Liberation Serif";
default_graph_appearance.axis_labels_font.size = 10.0;
default_graph_appearance.axis_labels_font.slant = CAIRO_FONT_SLANT_NORMAL;
default_graph_appearance.axis_labels_font.weight = CAIRO_FONT_WEIGHT_NORMAL;
default_graph_appearance.axis_labels_color.r = 1.0;
default_graph_appearance.axis_labels_color.g = 0.0;
default_graph_appearance.axis_labels_color.b = 0.0;
default_graph_appearance.traces_thickness = 2.0;
default_graph_appearance.traces_color.r = 0.2;
default_graph_appearance.traces_color.g = 0.6;
default_graph_appearance.traces_color.b = 1.0;
default_graph_appearance.distance_widget_borders_to_axis_labels = 4;
default_graph_appearance.distance_axis_labels_to_tick_labels = 1;
default_graph_appearance.distance_tick_labels_to_gridlines = 1;

//ПРИ ДВОЙНОМ ВЫЗОВЕ ТЕПЕРЬ НОРМАЛЬНО
graph_set_appearance(default_graph_appearance);	//(36)
graph_set_appearance(default_graph_appearance);	//(36)

char *y_tick_labels_2[] = {"10", "20", "30", "40", "50"};
char *x_tick_labels_2[] = {"10^0", "10^1", "10^2", "10^3", "10^4", "10^5"};

struct graph graph2;	//(36)
graph2.y_scale = LOG;
graph2.x_scale = LOG;
graph2.y_number_of_major_grids = 4;
graph2.x_number_of_major_grids = 5;
graph2.y_number_of_minor_grids = 6;
graph2.x_number_of_minor_grids = 12;
graph2.y_tick_labels = y_tick_labels_2;
graph2.x_tick_labels = x_tick_labels_2;
graph2.y_axis_label = "Ось Y";
graph2.x_axis_label = "Ось X";

/*g_print ("ИСХОДНЫЕ АДРЕСА:\n");
g_print ("Y TICK LABELS:\n");
unsigned int addr;
for (unsigned int i = 0; i <= graph2.y_number_of_major_grids; i++)
{
	g_print ("\tИтерация: %i\n", i);
	g_print ("\tАдрес начала строки: %p\n", &y_tick_labels_2[i][0]);
}
g_print ("X TICK LABELS:\n");
for (unsigned int i = 0; i <= graph2.x_number_of_major_grids; i++)
{
	g_print ("\tИтерация: %i\n", i);
	g_print ("\tАдрес начала строки: %p\n", &x_tick_labels_2[i][0]);
}
g_print ("Y AXIS LABEL:\n");
	g_print ("\tАдрес начала строки: %p\n", &graph2.y_axis_label[0]);
	//Выводим последовательность символов:
	g_print ("\tСписок символов:\n");
	for (int i = 0; i < 9; i++)
	{
		g_print ("\t\tASCII №%i: %c\n", graph2.y_axis_label[i], graph2.y_axis_label[i]);
	}
g_print ("X AXIS LABEL:\n");
	g_print ("\tАдрес начала строки: %p\n", &graph2.x_axis_label[0]);
	//Выводим последовательность символов:
	g_print ("\tСписок символов:\n");
	for (int i = 0; i < 9; i++)
	{
		g_print ("\t\tASCII №%i: %c\n", graph2.x_axis_label[i], graph2.x_axis_label[i]);
	}*/

g_print("Сейчас будет gtk_widget_show_all\n");

	gtk_widget_show_all(window);

//Графики обязательно нужно чертить после show_all (наверное хотябы area дб show)
//Иначе get_size_request выдает нули и всё по бороде
//Кроме того, если менять size_request в процессе выполнения проги не из graph, то нужно обновлять график функцией graph, чтобы учлись новые размеры
gtk_widget_set_size_request (drawing_area_graph2, 300, 100);
graph(GTK_DRAWING_AREA(drawing_area_graph2), graph2);	//(36)

/*//Пример получения size_request
GtkRequisition mins;
GtkRequisition nats;
//В начале там случайное число
g_print("minimum W:%d H:%d\n", mins.width, mins.height);
g_print("minimum W:%d H:%d\n", nats.width, nats.height);
gtk_widget_get_preferred_size (drawing_area_graph2, &mins, &nats);
g_print("minimum W:%d H:%d\n", mins.width, mins.height);
g_print("minimum W:%d H:%d\n", nats.width, nats.height);
gtk_widget_set_size_request (drawing_area_graph2, 100, 50);
gtk_widget_get_preferred_size (drawing_area_graph2, &mins, &nats);
g_print("minimum W:%d H:%d\n", mins.width, mins.height);
g_print("minimum W:%d H:%d\n", nats.width, nats.height);
*/

	//Обработка параметров, переданных программе:
	if (argc >= 2)
	{
		for (int i = 1; i < argc; i++)
		{
			if (strcmp(argv[i], "-ao") == 0)
			{
				file_open();
				break;
			}
		}
	}
g_print ("Сейчас будет gtk_main\n");
	gtk_main();
	return 0;
}
