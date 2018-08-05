//Модуль построения графиков (заголовочный файл)
//v0.001.

struct color {
	double r;
	double g;
	double b;
};

struct font {
	char *name;
	double size;
	cairo_font_slant_t slant;
	cairo_font_weight_t weight;
};

struct graph_appearance {
	struct color background_color;
	double major_gridlines_thickness;
	struct color major_gridlines_color;
	double minor_gridlines_thickness;
	struct color minor_gridlines_color;
	struct font tick_labels_font;
	struct color tick_labels_color;
	struct font axis_labels_font;
	struct color axis_labels_color;
	double traces_thickness;
	struct color traces_color;
	guint distance_widget_borders_to_axis_labels;
	guint distance_axis_labels_to_tick_labels;
	guint distance_tick_labels_to_gridlines;
};

enum scale {
	LIN,
	LOG
};

struct point {
	double x;
	double y;
};

struct graph {
	enum scale y_scale;
	enum scale x_scale;
	unsigned int y_number_of_major_grids;
	unsigned int x_number_of_major_grids;
	unsigned int y_number_of_minor_grids;
	unsigned int x_number_of_minor_grids;
	char *y_tick_labels;	///!!!
	char *x_tick_labels;	///!!!
	char *y_axis_label;
	char *x_axis_label;
	double y_value_min;
	double y_value_max;
	double x_value_min;
	double x_value_max;
	unsigned int trace_number_of_points;
	struct point *trace_points;
};

//!!!ГРАНИЦА ЧИСТОВОГО И ЧЕРНОВОГО КОДА:
void graph_appearance_set (struct graph_appearance new_graph_appearance);
void graph_plot (GtkDrawingArea *area, struct graph new_graph);
void graph_delete (GtkDrawingArea *area);
void graph_memory_free ();
//!!!Временно:
void char_array_print ();
