//Модуль построения графиков (заголовочный файл)
//v0.001.

struct font {
	char *name;
	double size;
	cairo_font_slant_t slant;
	cairo_font_weight_t weight;
};

struct color {
	double r;
	double g;
	double b;
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

//!!!ГРАНИЦА ЧИСТОВОГО И ЧЕРНОВОГО КОДА:

enum scale
{
	LIN,
	LOG
};

struct graph {
	enum scale y_scale;
	enum scale x_scale;
	unsigned int y_number_of_major_grids;
	unsigned int x_number_of_major_grids;
	unsigned int y_number_of_minor_grids;
	unsigned int x_number_of_minor_grids;
	void *y_tick_labels;
	void *x_tick_labels;	
	char *y_axis_label;
	char *x_axis_label;
};

void graph_memory_free ();
void graph_set_appearance (struct graph_appearance new_graph_appearance);
void graph (GtkDrawingArea *area, struct graph new_graph);
