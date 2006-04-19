
/* gapcmon_gtkglgraph.c    serial-0070-1 ***************************************
 *
 *  Modified from the original package called "GtkGLGraph" by Todd Goyen
 *      Thu Jun 10 22:50:18 2004
 *      Copyright  2004  Todd Goyen (GPL)
 *      tgoyen@swri.org or me@mumblelina.com
 *      http://mumblelina.com
 *
 * modified by James Scott, Jr.<skoona@users.sourceforge.net>
 *  March, 2006 (GPL)
 *    -  repacking for use in gapcmon.sourceforge.net
 *    -  enabled legend support and full tooltip support
 *    -  created a unrealize() and destroy() method.
 *    -  created a example program for the library showing linegraph
*/

#include <gtk/gtk.h>
#include "gapcmon_gtkglgraph.h"

#ifdef TEXT
#undef TEXT
#endif                             /* TEXT */
#define TEXT	 0.0
#define Z_WIDTH	40.0

#define WIDTH 		300
#define HEIGHT		300
#define PIXPERDIV	20
#define TOOLTIP_TIMEOUT	150

/* #define INSET		15.0 */
#define INSET       120.0
#define TTY_BORDER	6.0
#define TTX_BORDER	8.0

enum {
   SET_SCROLL_ADJUSTMENTS,
   CURSOR_MOVED,
   LAST_SIGNAL
};

/*
 * A few globals */
static GtkWidgetClass *parent_class = NULL;
static guint signals[LAST_SIGNAL] = { 0 };

/* end of globals
*/

#define g_marshal_value_peek_pointer(v)  g_value_get_pointer (v)
#define g_marshal_value_peek_object(v)   g_value_get_object (v)

static GdkPixbuf *gtk_glgraph_create_rotated_flipped_text_pixbuf(GtkGLGraph * glg,
   PangoLayout *
   layout, GtkGLGraphDirection dir, gboolean horizontal, gboolean vertical);

/* static gboolean gtk_glgraph_expose_timeout(GtkWidget * widget); */
static gboolean gtk_glgraph_tooltip_timeout(GtkGLGraph * const glg);
static gdouble BiCubicR(gdouble x);
static gint gtk_glgraph_button_press_event(GtkWidget * widget,
   GdkEventButton * event);
static gint gtk_glgraph_button_release_event(GtkWidget * widget,
   GdkEventButton * event);
static void gtk_glgraph_class_init(GtkGLGraphClass * class);
static gint gtk_glgraph_enter_notify_event(GtkWidget * widget,
   GdkEventCrossing * event);
static gint gtk_glgraph_expose(GtkWidget * widget, GdkEventExpose * event);
static gint gtk_glgraph_leave_notify_event(GtkWidget * widget,
   GdkEventCrossing * event);
static gint gtk_glgraph_motion_notify_event(GtkWidget * widget,
   GdkEventMotion * event);

/* static guint16  rotate (guint16 data); */
static void g_cclosure_user_marshal_VOID__OBJECT_OBJECT(GClosure * closure,
   GValue * return_value,
   guint n_param_values,
   const GValue * param_values, gpointer invocation_hint, gpointer marshal_data);
static void get_adjustments(GtkGLGraph * const glg);
static void gtk_glgraph_destroy(GtkObject * object);
static void gtk_glgraph_draw_cursor(const GtkGLGraph * glg);
static void gtk_glgraph_draw_cursor_boxes(const GtkGLGraph * glg);
static void gtk_glgraph_draw_cursor_horizontal(const GtkGLGraph * glg);
static void gtk_glgraph_draw_cursor_vertical(const GtkGLGraph * glg);
static void gtk_glgraph_draw_dataset(const GtkGLGraph * const glg,
   const GtkGLGDataSet * const glds);
static void gtk_glgraph_draw_dataset_surface(const GtkGLGraph * const glg,
   const GtkGLGDataSet * const glds);
static void gtk_glgraph_draw_dataset_xy(const GtkGLGraph * const glg,
   const GtkGLGDataSet * const glds);
static void gtk_glgraph_draw_gl_background(const GtkGLGraph * glg);
static void gtk_glgraph_draw_graph_box(const GtkGLGraph * glg);
static void gtk_glgraph_draw_legend(const GtkGLGraph * glg);
static void gtk_glgraph_draw_title(const GtkGLGraph * glg);
static void gtk_glgraph_draw_tooltip(GtkGLGraph * const glg,
   const GtkGLGDataSet * const glds);
static void gtk_glgraph_draw_x_axis(const GtkGLGraph * glg);
static void gtk_glgraph_draw_x_inside_tick_marks(const GtkGLGraph * glg);
static void gtk_glgraph_draw_x_major_grid(const GtkGLGraph * glg);
static void gtk_glgraph_draw_x_major_tick_text(const GtkGLGraph * glg);
static void gtk_glgraph_draw_x_minor_grid(const GtkGLGraph * glg);
static void gtk_glgraph_draw_x_outside_tick_marks(const GtkGLGraph * glg);
static void gtk_glgraph_draw_x_title(const GtkGLGraph * glg);
static void gtk_glgraph_draw_y_axis(const GtkGLGraph * glg);
static void gtk_glgraph_draw_y_inside_tick_marks(const GtkGLGraph * glg);
static void gtk_glgraph_draw_y_major_grid(const GtkGLGraph * glg);
static void gtk_glgraph_draw_y_major_tick_text(const GtkGLGraph * glg);
static void gtk_glgraph_draw_y_minor_grid(const GtkGLGraph * glg);
static void gtk_glgraph_draw_y_outside_tick_marks(const GtkGLGraph * glg);
static void gtk_glgraph_draw_y_title(const GtkGLGraph * glg);
static void gtk_glgraph_draw_z_axis(const GtkGLGraph * glg);
static void gtk_glgraph_draw_z_graph_box(const GtkGLGraph * glg);
static void gtk_glgraph_draw_z_inside_tick_marks(const GtkGLGraph * glg);
static void gtk_glgraph_draw_z_major_grid(const GtkGLGraph * glg);
static void gtk_glgraph_draw_z_major_tick_text(const GtkGLGraph * glg);
static void gtk_glgraph_draw_z_minor_grid(const GtkGLGraph * glg);
static void gtk_glgraph_draw_z_outside_tick_marks(const GtkGLGraph * glg);
static void gtk_glgraph_draw_z_scale(const GtkGLGraph * glg);
static void gtk_glgraph_draw_z_title(const GtkGLGraph * glg);
static void gtk_glgraph_init(GtkGLGraph * glg);
static void gtk_glgraph_layout_calculate_offsets(GtkGLGraph * glg);
static void gtk_glgraph_pixbuf_update_tooltip(GtkGLGraph * const glg,
   gchar * const text);
static void gtk_glgraph_realize(GtkWidget * widget);
static void gtk_glgraph_unrealize(GtkWidget * widget);
static void gtk_glgraph_set_color(const GtkGLGraph * const glg, gdouble z,
   gdouble alpha, GtkGLGraphColormap cmap, gdouble ma, gdouble mi);
static void gtk_glgraph_set_scroll_adjustments(GtkGLGraph * glg,
   GtkAdjustment * hadj, GtkAdjustment * vadj);
static void gtk_glgraph_size_allocate(GtkWidget * widget,
   GtkAllocation * allocation);
static void gtk_glgraph_size_request(GtkWidget * widget,
   GtkRequisition * requisition);
static void gtk_glgraph_update_axis_ticks(GtkGLGraph * const glg,
   GtkGLGraphAxis * const axis);
static void gtk_glgraph_value_changed(GtkAdjustment * adj, GtkGLGraph * glg);

/*
 * Start of code section
*/
static void gtk_glgraph_unrealize(GtkWidget * widget)
{
   GtkGLGraph *glg = NULL;

/*   GdkWindow *window = NULL; */

   g_return_if_fail(widget != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(widget));
   g_return_if_fail(widget->window != NULL);

   glg = GTK_GLGRAPH(widget);

   if (GTK_WIDGET_MAPPED(widget))
      gtk_widget_unmap(widget);

   if (glg->dpy != NULL) {
      glXDestroyContext(glg->dpy, glg->cx);
      glFinish();
      glg->dpy = NULL;
      glg->cx = NULL;
   }

   GTK_WIDGET_UNSET_FLAGS(widget, GTK_MAPPED);

/* SOMEHOW THESE ARE ALREADY DESTROYED AND THESE COMMAND ARE INVALID */

/*   window = GDK_WINDOW(widget->window); */

/*   if (window != NULL) { */

/*      gdk_window_set_user_data(window, NULL); */

/*      gdk_window_destroy(window); */

/*      widget->window = NULL; */

/*   } */

   if (GTK_WIDGET_CLASS(parent_class)->unrealize)
      (*GTK_WIDGET_CLASS(parent_class)->unrealize) (widget);

   return;
}

extern void gtk_glgraph_axis_set_label(GtkGLGraph * glg, GtkGLGraphAxisType axis,
   const gchar * label)
{

   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));
   g_return_if_fail(label != NULL);
   g_return_if_fail(g_utf8_strlen(label, -1) != 0);

   /* Return if the axis is out of range */
   if (axis >= GTKGLG_AXIS_LAST) {
      g_printerr("Invalid Axis\n");
      return;
   }

   /* Free the old label */
   if (glg->axes[axis].title) {
      g_free(glg->axes[axis].title);
   }

   /* Copy the new label */
   glg->axes[axis].title = g_strdup(label);

   /* Set the layout */
   if (GTK_WIDGET_REALIZED(GTK_WIDGET(glg)) == FALSE) {
      return;
   }

   pango_layout_set_markup(glg->axes[axis].title_layout, label, -1);

   if (glg->axes[axis].pixbuf != NULL) {
      g_object_unref(glg->axes[axis].pixbuf);
      glg->axes[axis].pixbuf = NULL;
   }

   /* Rotate the label if needed */
   switch (axis) {
   case GTKGLG_AXIS_X:
      glg->axes[axis].pixbuf =
         gtk_glgraph_create_rotated_flipped_text_pixbuf(glg,
         glg->axes[axis].title_layout, GTKGLG_DIRECTION_0, FALSE, TRUE);
      break;
   case GTKGLG_AXIS_Y:
      glg->axes[axis].pixbuf =
         gtk_glgraph_create_rotated_flipped_text_pixbuf(glg,
         glg->axes[axis].title_layout, GTKGLG_DIRECTION_90, TRUE, FALSE);
      break;
   case GTKGLG_AXIS_Z:
      glg->axes[axis].pixbuf =
         gtk_glgraph_create_rotated_flipped_text_pixbuf(glg,
         glg->axes[axis].title_layout, GTKGLG_DIRECTION_270, TRUE, FALSE);
      break;
   default:
      break;
   }
}

extern const gchar *gtk_glgraph_axis_get_label(const GtkGLGraph * glg,
   GtkGLGraphAxisType axis)
{
   g_return_val_if_fail(glg != NULL, NULL);

   g_return_val_if_fail(GTK_IS_GLGRAPH(glg), NULL);

   if (axis >= GTKGLG_AXIS_LAST) {
      g_printerr("Invalid Axis\n");
      return NULL;
   }

   return (glg->axes[axis].title);
}

extern void gtk_glgraph_axis_set_drawn(GtkGLGraph * glg, GtkGLGraphAxisType axis,
   const GtkGLGDrawAxis drawn)
{
   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));
   if (axis >= GTKGLG_AXIS_LAST) {
      return;
   }

   glg->axes[axis].drawn = drawn;
}

extern GtkGLGDrawAxis gtk_glgraph_axis_get_visible(GtkGLGraph * glg,
   GtkGLGraphAxisType axis)
{
   g_return_val_if_fail(glg != NULL, 0);
   g_return_val_if_fail(GTK_IS_GLGRAPH(glg), 0);
   if (axis >= GTKGLG_AXIS_LAST) {
      return 0;
   }

   return glg->axes[axis].drawn;
}

extern void gtk_glgraph_axis_set_range(GtkGLGraph * glg, GtkGLGraphAxisType axis,
   gdouble * min, gdouble * max,
   gint32 * major_steps, gint32 * minor_steps, gint8 * precision)
{
   gboolean resize = FALSE;
   GtkWidget *widget;

   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));
   if (axis >= GTKGLG_AXIS_LAST) {
      return;
   }

   if (min != NULL) {
      glg->axes[axis].min = *min;
   }
   if (max != NULL) {
      glg->axes[axis].max = *max;
   }
   if (major_steps != NULL) {
      glg->axes[axis].major_steps = *major_steps;
      resize = TRUE;
   }
   if (minor_steps != NULL) {
      glg->axes[axis].minor_steps = *minor_steps;
      resize = TRUE;
   }
   if (precision != NULL) {
      glg->axes[axis].precision = *precision;
   }

   if (resize == TRUE) {
      gtk_widget_queue_resize(GTK_WIDGET(glg));
   }

   widget = GTK_WIDGET(glg);
   if (GTK_WIDGET_REALIZED(widget)) {
      gtk_glgraph_update_axis_ticks(glg, &(glg->axes[axis]));
   }
}

extern void gtk_glgraph_axis_get_range(GtkGLGraph * glg, GtkGLGraphAxisType axis,
   gdouble * min, gdouble * max,
   gint32 * major_steps, gint32 * minor_steps, gint8 * precision)
{
   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   if (axis >= GTKGLG_AXIS_LAST) {
      return;
   }

   if (min != NULL) {
      *min = glg->axes[axis].min;
   }
   if (max != NULL) {
      *max = glg->axes[axis].max;
   }
   if (major_steps != NULL) {
      *major_steps = glg->axes[axis].major_steps;
   }
   if (minor_steps != NULL) {
      *minor_steps = glg->axes[axis].minor_steps;
   }
   if (precision != NULL) {
      *precision = glg->axes[axis].precision;
   }
}

extern void gtk_glgraph_axis_set_mode(GtkGLGraph * const glg,
   GtkGLGraphAxisMode mode)
{
   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));
   glg->mode = mode;
}

extern GtkGLGraphAxisMode gtk_glgraph_axis_get_mode(const GtkGLGraph * const glg)
{
   g_return_val_if_fail(glg != NULL, -1);
   g_return_val_if_fail(GTK_IS_GLGRAPH(glg), -1);
   return (glg->mode);
}

extern GType gtk_glgraph_get_type(void)
{
   static GType glgraph_type = 0;

   if (!glgraph_type) {
      static const GTypeInfo glgraph_info = {
         sizeof(GtkGLGraphClass),
         NULL,                     /* base_init */
         NULL,                     /* base_finalize */
         (GClassInitFunc) gtk_glgraph_class_init,
         NULL,                     /* class_finalize */
         NULL,                     /* class_data */
         sizeof(GtkGLGraph),
         0,                        /* n_preallocs */
         (GInstanceInitFunc) gtk_glgraph_init,
         NULL
      };
      glgraph_type =
         g_type_register_static(gtk_widget_get_type(), "GtkGLGraph",
         &glgraph_info, 0);
   }
   return glgraph_type;
}

static void gtk_glgraph_class_init(GtkGLGraphClass * class)
{
   GtkObjectClass *object_class = GTK_OBJECT_CLASS(class);
   GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(class);

   parent_class = gtk_type_class(gtk_widget_get_type());

   object_class->destroy = gtk_glgraph_destroy;

   widget_class->size_allocate = gtk_glgraph_size_allocate;
   widget_class->size_request = gtk_glgraph_size_request;
   widget_class->expose_event = gtk_glgraph_expose;
   widget_class->realize = gtk_glgraph_realize;
   widget_class->unrealize = gtk_glgraph_unrealize;
   widget_class->button_press_event = gtk_glgraph_button_press_event;
   widget_class->button_release_event = gtk_glgraph_button_release_event;
   widget_class->motion_notify_event = gtk_glgraph_motion_notify_event;
   widget_class->enter_notify_event = gtk_glgraph_enter_notify_event;
   widget_class->leave_notify_event = gtk_glgraph_leave_notify_event;

   class->set_scroll_adjustments = gtk_glgraph_set_scroll_adjustments;

   signals[SET_SCROLL_ADJUSTMENTS] =
      g_signal_new("set_scroll_adjustments", G_OBJECT_CLASS_TYPE(object_class),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
      G_STRUCT_OFFSET(GtkGLGraphClass, set_scroll_adjustments),
      NULL, NULL, g_cclosure_user_marshal_VOID__OBJECT_OBJECT,
      G_TYPE_NONE, 2, GTK_TYPE_ADJUSTMENT, GTK_TYPE_ADJUSTMENT);
   signals[CURSOR_MOVED] =
      g_signal_new("cursor_moved", G_OBJECT_CLASS_TYPE(object_class),
      G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(GtkGLGraphClass,
         cursor_moved), NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

   widget_class->set_scroll_adjustments_signal = signals[SET_SCROLL_ADJUSTMENTS];
}

static void gtk_glgraph_init(GtkGLGraph * glg)
{
   gint i;

   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   /* We double buffer opengl internally */
   gtk_widget_set_double_buffered(GTK_WIDGET(glg), FALSE);

   glg->mx_last = 0;
   glg->my_last = 0;

   /* Draw Everything */
   glg->drawn = GTKGLG_D_TITLE;
   glg->legend_in_out = FALSE;
   glg->tooltip_visible = FALSE;
   glg->tooltip_id_valid = FALSE;
   glg->tooltip_id = 0;
   glg->ttx = 0.0;
   glg->tty = 0.0;
   glg->datasets = NULL;
   glg->mode = GTKGLG_AXIS_MODE_EQUAL;
   glg->top = 0.0;
   glg->bottom = 0.0;
   glg->left = 0.0;
   glg->right = 0.0;
   glg->z_left = 0.0;
   glg->z_right = 0.0;
   glg->l_left = 0.0;
   glg->legend_text_width = 0.0;
   glg->w = 0.0;
   glg->h = 0.0;
   glg->title = g_strdup("");
   glg->title_layout = NULL;
   glg->title_pixbuf = NULL;
   glg->tooltip_pixbuf = NULL;
   glg->cursor_stipple = 0xF0F0;
   glg->cursor_moving = FALSE;
   glg->cx1 = 0.0;
   glg->cy1 = 0.0;
   glg->cx2 = 0.0;
   glg->cy2 = 0.0;
   glg->hadj = NULL;
   glg->vadj = NULL;
   glg->width = 0;
   glg->height = 0;
   glg->xoffset = 0;
   glg->yoffset = 0;
   glg->zoom = 1.0;
   glg->queued_draw = 0;

   /* Initialize the Axes */
   for (i = 0; i < GTKGLG_AXIS_LAST; i++) {
      glg->axes[i].min = -2.5;
      glg->axes[i].max = 2.5;
      glg->axes[i].major_steps = 10;
      glg->axes[i].minor_steps = 5;
      glg->axes[i].precision = 1;
      glg->axes[i].title = g_strdup("Axis");
      glg->axes[i].title_layout = NULL;
      glg->axes[i].pixbuf = NULL;
      glg->axes[i].major_tick_pixbuf_list = NULL;
      glg->axes[i].drawn =
         GTKGLG_DA_AXIS | GTKGLG_DA_INSIDE_TICK_MARKS |
         GTKGLG_DA_OUTSIDE_TICK_MARKS | GTKGLG_DA_MAJOR_GRID |
         GTKGLG_DA_MINOR_GRID | GTKGLG_DA_TITLE | GTKGLG_DA_MAJOR_TICK_TEXT;
   }
}

extern GtkWidget *gtk_glgraph_new(void)
{
   GtkGLGraph *glg;

   glg = (GtkGLGraph *) gtk_type_new(gtk_glgraph_get_type());

   return GTK_WIDGET(glg);
}

static void gtk_glgraph_destroy(GtkObject * object)
{
   GtkGLGraph *glg = NULL;

   g_return_if_fail(object != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(object));

   glg = GTK_GLGRAPH(object);

   if (glg != NULL) {
      if (glg->tooltip_id) {
         g_source_remove(glg->tooltip_id);
         glg->tooltip_id_valid = FALSE;
         glg->tooltip_visible = FALSE;
         glg->tooltip_id = 0;
      }
      if (glg->expose_id) {
         g_source_remove(glg->expose_id);
         glg->expose_id = 0;
      }
      gtk_glgraph_dataset_free_all(glg);
   }

   if (GTK_OBJECT_CLASS(parent_class)->destroy != NULL) {
      (*GTK_OBJECT_CLASS(parent_class)->destroy) (object);
   }

}

static void gtk_glgraph_size_request(GtkWidget * widget,
   GtkRequisition * requisition)
{
   GtkGLGraph *glg;
   GtkGLGraphAxis *axis;
   gint32 mi;
   gint32 ma;

   g_return_if_fail(widget != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(widget));
   g_return_if_fail(requisition != NULL);

   glg = GTK_GLGRAPH(widget);

   /* Reset Border Spacings */
   gtk_glgraph_layout_calculate_offsets(glg);

   /* Figure out requisition */
   if (glg->mode != GTKGLG_AXIS_MODE_FILL) {
      /* Old resize function == BAD */
      /* Graph shape dependent on steps */
      /* Replace with aspect ratio call TO BE WRITTEN */
      axis = &(glg->axes[GTKGLG_AXIS_X]);
      mi = axis->minor_steps <= 0 ? 1 : axis->minor_steps;
      ma = axis->major_steps <= 0 ? 1 : axis->major_steps;
      requisition->width =
         (gint) (glg->zoom * ((gdouble) (PIXPERDIV * mi * ma))) + glg->left +
         glg->right;

      axis = &(glg->axes[GTKGLG_AXIS_Y]);
      mi = axis->minor_steps <= 0 ? 1 : axis->minor_steps;
      ma = axis->major_steps <= 0 ? 1 : axis->major_steps;
      requisition->height =
         (gint) (glg->zoom * ((gdouble) (PIXPERDIV * mi * ma))) + glg->top +
         glg->bottom;

      glg->width = requisition->width;
      glg->height = requisition->height;
   }

}

static void get_adjustments(GtkGLGraph * const glg)
{
   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   if (glg->hadj == NULL && glg->vadj == NULL) {
      gtk_glgraph_set_scroll_adjustments(glg, NULL, NULL);
   } else {
      if (glg->hadj == NULL) {
         gtk_glgraph_set_scroll_adjustments(glg, NULL, glg->vadj);
      }
      if (glg->vadj == NULL) {
         gtk_glgraph_set_scroll_adjustments(glg, glg->hadj, NULL);
      }
   }
}

static void gtk_glgraph_size_allocate(GtkWidget * widget, GtkAllocation * allocation)
{
   GtkGLGraph *glg = NULL;

   g_return_if_fail(widget != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(widget));
   g_return_if_fail(allocation != NULL);

   /* Get size from our parent */
   (*GTK_WIDGET_CLASS(parent_class)->size_allocate) (widget, allocation);

   glg = GTK_GLGRAPH(widget);

   widget->allocation = *allocation;

   if (GTK_WIDGET_REALIZED(widget)) {
      /* Resize window */
      gdk_window_move_resize(widget->window, allocation->x, allocation->y,
         allocation->width, allocation->height);
   }

   if (glg->mode == GTKGLG_AXIS_MODE_FILL) {
      glg->width = allocation->width;
      glg->height = allocation->height;
   }

   /* Ensure adjustments exist */
   get_adjustments(glg);

   /* Setup horizontal adjustment */
   glg->hadj->page_size = allocation->width;
   glg->hadj->page_increment = allocation->width * 0.9;
   glg->hadj->step_increment = allocation->width * 0.1;
   glg->hadj->lower = 0;
   glg->hadj->upper = (gdouble) MAX(allocation->width, glg->width);

   if (glg->hadj->value > glg->hadj->upper - glg->hadj->page_size) {
      gtk_adjustment_set_value(glg->hadj,
         MAX(0, glg->hadj->upper - glg->hadj->page_size));
   }

   gtk_adjustment_changed(glg->hadj);

   glg->vadj->page_size = allocation->height;
   glg->vadj->page_increment = allocation->height * 0.9;
   glg->vadj->step_increment = allocation->height * 0.1;
   glg->vadj->lower = 0;
   glg->vadj->upper = (gdouble) MAX(allocation->height, glg->height);

   if (glg->vadj->value > glg->vadj->upper - glg->vadj->page_size) {
      gtk_adjustment_set_value(glg->vadj,
         MAX(0, glg->vadj->upper - glg->vadj->page_size));
   }

   gtk_adjustment_changed(glg->vadj);

   if (GTK_WIDGET_REALIZED(widget)) {
      GLdouble x, y, w, h;

#ifdef G_OS_UNIX
      glXWaitX();                  /* This is important */
      glXMakeCurrent(glg->dpy, GDK_DRAWABLE_XID(widget->window), glg->cx);
#endif                             /* linux */
#ifdef G_OS_WIN32
      wglMakeCurrent(glg->hDC, glg->hRC);
#endif                             /* WINDOWS */

      /* Get Window Sizes */
      x = (GLdouble) glg->hadj->value;
      y = (GLdouble) (((gdouble) glg->height) - glg->vadj->value);
      w = (GLdouble) widget->allocation.width;
      h = (GLdouble) widget->allocation.height;
      /* Fix y for OpenGL coordinate system */
      y = y - h;

      glViewport(0, 0, (GLint) w, (GLint) h);
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      /* Clamp the window to whats visible */
      gluOrtho2D(x, x + w, y, y + h);
   }

}

/*
static gboolean gtk_glgraph_expose_timeout(GtkWidget * widget)
{
   GtkGLGraph *glg;

   g_return_val_if_fail(widget != NULL, FALSE);
   g_return_val_if_fail(GTK_IS_GLGRAPH(widget), FALSE);

   glg = GTK_GLGRAPH(widget);

   glg->cursor_stipple = rotate(glg->cursor_stipple);

   --* Draw the cursor if asked too and it has a nonzero span *--
   if (glg->drawn & GTKGLG_D_CURSOR_BOX && glg->cx1 != glg->cx2 &&
      glg->cy1 != glg->cy2) {
      gtk_widget_queue_draw(widget);
   }

   return TRUE;
}


static guint16 rotate (guint16 data)
{
  guint16         carry;

  carry = data & 0x8000;
  carry = carry >> 15;
  data = data << 1;
  data = data | carry;

  return data;
}
*/

static gint gtk_glgraph_expose(GtkWidget * widget, GdkEventExpose * event)
{
   GtkGLGraph *glg;
   GList *datasets;
   GtkGLGDataSet *dataset;

   g_return_val_if_fail(widget != NULL, FALSE);
   g_return_val_if_fail(GTK_IS_GLGRAPH(widget), FALSE);

   /* Only Draw if we need too */
   if (!(GTK_WIDGET_VISIBLE(widget) && GTK_WIDGET_MAPPED(widget))) {
      return FALSE;
   }

   glg = GTK_GLGRAPH(widget);
   glg->queued_draw = 0;

#ifdef G_OS_UNIX
   glXMakeCurrent(glg->dpy, GDK_DRAWABLE_XID(widget->window), glg->cx);
#endif                             /* linux */
#ifdef G_OS_WIN32
   wglMakeCurrent(glg->hDC, glg->hRC);
#endif                             /* WINDOWS */

   /* Get Width and Height */
   glg->w = (gdouble) (glg->width);
   glg->h = (gdouble) (glg->height);

   /* Reset Border Spacings */
   gtk_glgraph_layout_calculate_offsets(glg);

   /* Draw Widget */
   /* OpenGL Stuff */
   /* Clear the background */
   gtk_glgraph_draw_gl_background(glg);

   /* Draw the x axis */
   gtk_glgraph_draw_x_axis(glg);

   /* Draw the y axis */
   gtk_glgraph_draw_y_axis(glg);

   /* Draw the z axis */
   gtk_glgraph_draw_z_axis(glg);

   /* Enable Line Smoothing */
   glEnable(GL_LINE_SMOOTH);
   glEnable(GL_BLEND);
   /* Draw the Data */
   datasets = g_list_first(glg->datasets);
   while (datasets) {
      dataset = datasets->data;
      datasets = datasets->next;
      gtk_glgraph_draw_dataset(glg, dataset);
   }

   /* Draw the tooltip */
   datasets = g_list_first(glg->datasets);
   if (datasets) {
      dataset = datasets->data;
      gtk_glgraph_draw_tooltip(glg, dataset);
   }

   gtk_glgraph_draw_legend(glg);

   /* Disable Line Smoothing */
   glDisable(GL_LINE_SMOOTH);
   glDisable(GL_BLEND);

   /* Draw the Graph Box Border */
   gtk_glgraph_draw_graph_box(glg);

   gtk_glgraph_draw_cursor(glg);

   /* Draw the title */
   gtk_glgraph_draw_title(glg);

   /* Flush to the buffer and draw to screen */
#ifdef G_OS_UNIX
   glXSwapBuffers(glg->dpy, GDK_DRAWABLE_XID(widget->window));
#endif                             /* linux */
#ifdef G_OS_WIN32
   SwapBuffers(glg->hDC);
#endif                             /* WINDOWS */

   return FALSE;
}

static void gtk_glgraph_realize(GtkWidget * widget)
{
   GtkGLGraph *glg;
   GdkWindowAttr attributes;
   gint attributes_mask;
   GdkScreen *screen;

#ifdef G_OS_UNIX
   static gint attributeListDbl[] =
      { GLX_RGBA, GLX_DOUBLEBUFFER, GLX_RED_SIZE, 1, GLX_GREEN_SIZE, 1,
      GLX_BLUE_SIZE, 1, None
   };
#endif                             /* linux */
#ifdef G_OS_WIN32
   PIXELFORMATDESCRIPTOR pfd;
   gint iFormat;
#endif                             /* WINDOWS */

   g_return_if_fail(widget != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(widget));

   glg = GTK_GLGRAPH(widget);

   /* OpenGL Stuff */
   /* Get display */
#ifdef G_OS_UNIX

/*   glg->dpy = GDK_DISPLAY (); */
   screen = gdk_screen_get_default();
   glg->dpy = GDK_SCREEN_XDISPLAY(screen);

   /* Get a Double Buffer'd Visual for OpenGL */
   if (!glXQueryExtension(glg->dpy, NULL, NULL)) {
      g_error("glX not supported on this display, aborting...\n");
      return;
      /* exit (1); */
   }
   glg->vi = glXChooseVisual(glg->dpy, DefaultScreen(glg->dpy), attributeListDbl);
   glg->colormap = gdk_colormap_new(gdkx_visual_get(glg->vi->visualid), FALSE);
   glg->cx = glXCreateContext(glg->dpy, glg->vi, 0, GL_TRUE);
#endif                             /* linux */

   /* we set up the main widget! */
   GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);
   attributes.window_type = GDK_WINDOW_CHILD;
   attributes.x = widget->allocation.x;
   attributes.y = widget->allocation.y;
   attributes.width = widget->allocation.width;
   attributes.height = widget->allocation.height;
   attributes.wclass = GDK_INPUT_OUTPUT;
#ifdef G_OS_UNIX
   attributes.visual = gdkx_visual_get(glg->vi->visualid);
   attributes.colormap = glg->colormap;
#endif                             /* linux */
#ifdef G_OS_WIN32
   attributes.visual = gtk_widget_get_visual(widget);
   attributes.colormap = gtk_widget_get_colormap(widget);
#endif                             /* WINDOWS */
   attributes.event_mask =
      gtk_widget_get_events(widget) | GDK_EXPOSURE_MASK | GDK_STRUCTURE_MASK |
      GDK_POINTER_MOTION_MASK | GDK_FOCUS_CHANGE_MASK |
      GDK_VISIBILITY_NOTIFY_MASK | GDK_BUTTON_PRESS_MASK |
      GDK_BUTTON_RELEASE_MASK | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK;
   attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

   widget->window =
      gdk_window_new(gtk_widget_get_parent_window(widget), &attributes,
      attributes_mask);
   gdk_window_set_user_data(widget->window, widget);
   widget->style = gtk_style_attach(widget->style, widget->window);

   /* Set up the OpenGL Defaults */
   /* Use this context */
#ifdef G_OS_UNIX
   glXMakeCurrent(glg->dpy, GDK_DRAWABLE_XID(widget->window), glg->cx);
#endif                             /* linux */

#ifdef G_OS_WIN32
   glg->hDC = GetDC(GDK_WINDOW_HWND(widget->window));

   ZeroMemory(&pfd, sizeof(pfd));
   pfd.nSize = sizeof(pfd);
   pfd.nVersion = 1;
   pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
   pfd.iPixelType = PFD_TYPE_RGBA;
   pfd.cColorBits = 24;
   pfd.cDepthBits = 16;
   pfd.iLayerType = PFD_MAIN_PLANE;
   iFormat = ChoosePixelFormat(glg->hDC, &pfd);
   SetPixelFormat(glg->hDC, iFormat, &pfd);

   /* create and enable the render context (RC) */
   glg->hRC = wglCreateContext(glg->hDC);
   wglMakeCurrent(glg->hDC, glg->hRC);
#endif                             /* WINDOWS */

   /* Setup the widget view */
   glViewport(0, 0, widget->allocation.width, widget->allocation.height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluOrtho2D(0.0, (GLdouble) widget->allocation.width, 0.0,
      (GLdouble) widget->allocation.height);

   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
   glHint(GL_POLYGON_SMOOTH_HINT, GL_FASTEST);
   glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
   /* Disable Line Smoothing for now */
   glDisable(GL_LINE_SMOOTH);
   glDisable(GL_POINT_SMOOTH);
   glDisable(GL_BLEND);
   glDisable(GL_POLYGON_SMOOTH);
   glDisable(GL_LIGHTING);
   glShadeModel(GL_FLAT);

   /* Setup Title */
   glg->title_layout = gtk_widget_create_pango_layout(widget, NULL);
   pango_layout_set_alignment(glg->title_layout, PANGO_ALIGN_CENTER);
   pango_layout_set_markup(glg->title_layout, glg->title, -1);
   glg->title_pixbuf =
      gtk_glgraph_create_rotated_flipped_text_pixbuf(glg, glg->title_layout,
      GTKGLG_DIRECTION_0, FALSE, TRUE);

   /* Init X Axis */
   glg->axes[GTKGLG_AXIS_X].title_layout =
      gtk_widget_create_pango_layout(widget, NULL);
   pango_layout_set_markup(glg->axes[GTKGLG_AXIS_X].title_layout,
      glg->axes[GTKGLG_AXIS_X].title, -1);
   glg->axes[GTKGLG_AXIS_X].pixbuf =
      gtk_glgraph_create_rotated_flipped_text_pixbuf(glg,
      glg->axes[GTKGLG_AXIS_X].title_layout, GTKGLG_DIRECTION_0, FALSE, TRUE);
   gtk_glgraph_update_axis_ticks(glg, &(glg->axes[GTKGLG_AXIS_X]));
   /* Init Y Axis */
   glg->axes[GTKGLG_AXIS_Y].title_layout =
      gtk_widget_create_pango_layout(widget, NULL);
   pango_layout_set_markup(glg->axes[GTKGLG_AXIS_Y].title_layout,
      glg->axes[GTKGLG_AXIS_Y].title, -1);
   glg->axes[GTKGLG_AXIS_Y].pixbuf =
      gtk_glgraph_create_rotated_flipped_text_pixbuf(glg,
      glg->axes[GTKGLG_AXIS_Y].title_layout, GTKGLG_DIRECTION_90, TRUE, FALSE);
   gtk_glgraph_update_axis_ticks(glg, &(glg->axes[GTKGLG_AXIS_Y]));
   /* Init Z Axis */
   glg->axes[GTKGLG_AXIS_Z].title_layout =
      gtk_widget_create_pango_layout(widget, NULL);
   pango_layout_set_markup(glg->axes[GTKGLG_AXIS_Z].title_layout,
      glg->axes[GTKGLG_AXIS_Z].title, -1);
   glg->axes[GTKGLG_AXIS_Z].pixbuf =
      gtk_glgraph_create_rotated_flipped_text_pixbuf(glg,
      glg->axes[GTKGLG_AXIS_Z].title_layout, GTKGLG_DIRECTION_270, TRUE, FALSE);
   gtk_glgraph_update_axis_ticks(glg, &(glg->axes[GTKGLG_AXIS_Z]));

/* NOT NEEDED
   glg->expose_id =
      g_timeout_add(150, (GSourceFunc) gtk_glgraph_expose_timeout, widget);
*/

   gtk_widget_queue_resize(widget);
}

static gint gtk_glgraph_button_press_event(GtkWidget * widget,
   GdkEventButton * event)
{

   g_return_val_if_fail(widget != NULL, FALSE);
   g_return_val_if_fail(GTK_IS_GLGRAPH(widget), FALSE);

   if (event->button == 1) {
      GtkGLGraph *glg;
      gdouble xpo, xppu, ypo, yppu;     /* x pixel offset, x pixels per unit, etc... */

      glg = GTK_GLGRAPH(widget);

      /* Get Width and Height */
      glg->w = (gdouble) (glg->width);
      glg->h = (gdouble) (glg->height);

      /* Reset Border Spacings */
      gtk_glgraph_layout_calculate_offsets(glg);

      /* Figure out scaling */
      xppu = fabs((glg->w - glg->left - glg->right) / (glg->axes[GTKGLG_AXIS_X].max -
            glg->axes[GTKGLG_AXIS_X].min));
      yppu = fabs((glg->h - glg->bottom - glg->top) / (glg->axes[GTKGLG_AXIS_Y].max -
            glg->axes[GTKGLG_AXIS_Y].min));
      xpo = glg->left - glg->axes[GTKGLG_AXIS_X].min * xppu;
      ypo = glg->bottom - glg->axes[GTKGLG_AXIS_Y].min * yppu;

      glg->cursor_moving = TRUE;
      glg->cx1 = (event->x + glg->hadj->value - xpo) / xppu;
      glg->cy1 = (glg->h - event->y - glg->vadj->value - ypo) / yppu;
      /* Force the cursor onto the graph */
      glg->cx1 =
         CLAMP(glg->cx1, glg->axes[GTKGLG_AXIS_X].min, glg->axes[GTKGLG_AXIS_X].max);
      glg->cy1 =
         CLAMP(glg->cy1, glg->axes[GTKGLG_AXIS_Y].min, glg->axes[GTKGLG_AXIS_Y].max);
      /* Set the end to the start for now */
      glg->cx2 = glg->cx1;
      glg->cy2 = glg->cy1;

      gtk_widget_queue_draw(widget);
      return TRUE;
   }

   return FALSE;
}

static gint gtk_glgraph_button_release_event(GtkWidget * widget,
   GdkEventButton * event)
{
   g_return_val_if_fail(widget != NULL, FALSE);
   g_return_val_if_fail(GTK_IS_GLGRAPH(widget), FALSE);

   if (event->button == 1) {
      GtkGLGraph *glg;
      gdouble xpo, xppu, ypo, yppu;     /* x pixel offset, x pixels per unit, etc... */

      glg = GTK_GLGRAPH(widget);

      /* Get Width and Height */
      glg->w = (gdouble) (glg->width);
      glg->h = (gdouble) (glg->height);

      /* Reset Border Spacings */
      gtk_glgraph_layout_calculate_offsets(glg);

      /* Get Scaling */
      xppu = fabs((glg->w - glg->left - glg->right) / (glg->axes[GTKGLG_AXIS_X].max -
            glg->axes[GTKGLG_AXIS_X].min));
      yppu = fabs((glg->h - glg->bottom - glg->top) / (glg->axes[GTKGLG_AXIS_Y].max -
            glg->axes[GTKGLG_AXIS_Y].min));
      xpo = glg->left - glg->axes[GTKGLG_AXIS_X].min * xppu;
      ypo = glg->bottom - glg->axes[GTKGLG_AXIS_Y].min * yppu;

      /* Move cursor */
      glg->cursor_moving = FALSE;
      glg->cx2 = (event->x + glg->hadj->value - xpo) / xppu;
      glg->cy2 = (glg->h - event->y - glg->vadj->value - ypo) / yppu;
      glg->cx2 =
         CLAMP(glg->cx2, glg->axes[GTKGLG_AXIS_X].min, glg->axes[GTKGLG_AXIS_X].max);
      glg->cy2 =
         CLAMP(glg->cy2, glg->axes[GTKGLG_AXIS_Y].min, glg->axes[GTKGLG_AXIS_Y].max);

      gtk_widget_queue_draw(widget);
      g_signal_emit(glg, signals[CURSOR_MOVED], 0);
      return TRUE;
   }

   return FALSE;
}

static gint gtk_glgraph_motion_notify_event(GtkWidget * widget,
   GdkEventMotion * event)
{
   GtkGLGraph *glg = NULL;

   g_return_val_if_fail(widget != NULL, FALSE);
   g_return_val_if_fail(event != NULL, FALSE);
   g_return_val_if_fail(GTK_IS_GLGRAPH(widget), FALSE);

   glg = GTK_GLGRAPH(widget);
   g_return_val_if_fail(glg != NULL, FALSE);

   if (glg->tooltip_id_valid == TRUE) {
      g_source_remove(glg->tooltip_id);
   }
   if (glg->tooltip_visible == TRUE) {
      glg->tooltip_visible = FALSE;
      gtk_widget_queue_draw(GTK_WIDGET(glg));
   }

   if (glg->cursor_moving == TRUE) {
      GtkGLGraph *glg;
      gdouble xpo, xppu, ypo, yppu;     /* x pixel offset, x pixels per unit, etc... */

      glg = GTK_GLGRAPH(widget);

      /* Get Width and Height */
      glg->w = (gdouble) (glg->width);
      glg->h = (gdouble) (glg->height);

      /* Reset Border Spacings */
      gtk_glgraph_layout_calculate_offsets(glg);

      /* Get Scaling */
      xppu = fabs((glg->w - glg->left - glg->right) / (glg->axes[GTKGLG_AXIS_X].max -
            glg->axes[GTKGLG_AXIS_X].min));
      yppu = fabs((glg->h - glg->bottom - glg->top) / (glg->axes[GTKGLG_AXIS_Y].max -
            glg->axes[GTKGLG_AXIS_Y].min));
      xpo = glg->left - glg->axes[GTKGLG_AXIS_X].min * xppu;
      ypo = glg->bottom - glg->axes[GTKGLG_AXIS_Y].min * yppu;

      /* Move cursor */
      glg->cx2 = (event->x + glg->hadj->value - xpo) / xppu;
      glg->cy2 = (glg->h - event->y - glg->vadj->value - ypo) / yppu;
      glg->cx2 =
         CLAMP(glg->cx2, glg->axes[GTKGLG_AXIS_X].min, glg->axes[GTKGLG_AXIS_X].max);
      glg->cy2 =
         CLAMP(glg->cy2, glg->axes[GTKGLG_AXIS_Y].min, glg->axes[GTKGLG_AXIS_Y].max);

      gtk_widget_queue_draw(widget);
      return TRUE;
   }

   glg->tooltip_id_valid = TRUE;
   glg->tooltip_id =
      g_timeout_add(TOOLTIP_TIMEOUT, (GtkFunction) gtk_glgraph_tooltip_timeout, glg);
   glg->ttx = event->x;
   glg->tty = event->y;

   return FALSE;
}

static gint gtk_glgraph_enter_notify_event(GtkWidget * widget,
   GdkEventCrossing * event)
{
   GtkGLGraph *glg;

   g_return_val_if_fail(widget != NULL, FALSE);
   g_return_val_if_fail(GTK_IS_GLGRAPH(widget), FALSE);

   glg = GTK_GLGRAPH(widget);

   if (glg->tooltip_id_valid == TRUE) {
      g_source_remove(glg->tooltip_id);
   }
   if (glg->tooltip_visible == TRUE) {
      glg->tooltip_visible = FALSE;
      gtk_widget_queue_draw(GTK_WIDGET(glg));
   }
   glg->tooltip_id_valid = TRUE;
   glg->tooltip_id =
      g_timeout_add(TOOLTIP_TIMEOUT, (GtkFunction) gtk_glgraph_tooltip_timeout, glg);
   glg->ttx = event->x;
   glg->tty = event->y;

   return FALSE;
}

static gint gtk_glgraph_leave_notify_event(GtkWidget * widget,
   GdkEventCrossing * event)
{
   GtkGLGraph *glg;

   g_return_val_if_fail(widget != NULL, FALSE);
   g_return_val_if_fail(GTK_IS_GLGRAPH(widget), FALSE);

   glg = GTK_GLGRAPH(widget);

   if (glg->tooltip_id_valid == TRUE) {
      g_source_remove(glg->tooltip_id);
   }
   if (glg->tooltip_visible == TRUE) {
      glg->tooltip_visible = FALSE;
      gtk_widget_queue_draw(GTK_WIDGET(glg));
   }
   glg->tooltip_id_valid = FALSE;

   return FALSE;
}

static gboolean gtk_glgraph_tooltip_timeout(GtkGLGraph * const glg)
{
   g_return_val_if_fail(glg != NULL, FALSE);
   g_return_val_if_fail(GTK_IS_GLGRAPH(glg), FALSE);

   glg->tooltip_id_valid = FALSE;
   glg->tooltip_visible = TRUE;

   gtk_widget_queue_draw(GTK_WIDGET(glg));

   return FALSE;
}

static void gtk_glgraph_set_scroll_adjustments(GtkGLGraph * glg,
   GtkAdjustment * hadj, GtkAdjustment * vadj)
{
   gboolean need_adjust = FALSE;

   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   /* Check or Create the Adjustments */
   if (hadj) {
      g_return_if_fail(GTK_IS_ADJUSTMENT(hadj));
   } else {
      hadj = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
   }

   if (vadj) {
      g_return_if_fail(GTK_IS_ADJUSTMENT(vadj));
   } else {
      vadj = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
   }

   /* Remove old Adjustments if they aren't the ones passed to us */
   if (glg->hadj && (glg->hadj != hadj)) {
      g_signal_handlers_disconnect_by_func(glg->hadj,
         (GSourceFunc) gtk_glgraph_value_changed, glg);
      g_object_unref(glg->hadj);
   }

   if (glg->vadj && (glg->vadj != vadj)) {
      g_signal_handlers_disconnect_by_func(glg->vadj,
         (GSourceFunc) gtk_glgraph_value_changed, glg);
      g_object_unref(glg->vadj);
   }

   /* Install signals for new Adjustments */
   if (glg->hadj != hadj) {
      glg->hadj = hadj;
      g_object_ref(glg->hadj);
      gtk_object_sink(GTK_OBJECT(glg->hadj));
      g_signal_connect(glg->hadj, "value_changed",
         G_CALLBACK(gtk_glgraph_value_changed), glg);
      need_adjust = TRUE;
   }

   if (glg->vadj != vadj) {
      glg->vadj = vadj;
      g_object_ref(glg->vadj);
      gtk_object_sink(GTK_OBJECT(glg->vadj));
      g_signal_connect(glg->vadj, "value_changed",
         G_CALLBACK(gtk_glgraph_value_changed), glg);
      need_adjust = TRUE;
   }

   if (need_adjust) {
      gtk_glgraph_value_changed(NULL, glg);
   }
}

static void gtk_glgraph_value_changed(GtkAdjustment * adj, GtkGLGraph * glg)
{
   gint dx = 0;
   gint dy = 0;
   GtkWidget *widget;
   GLdouble x, y, w, h;

   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   widget = GTK_WIDGET(glg);

   if (adj == glg->hadj) {
      dx = glg->xoffset - (gint) adj->value;
      glg->xoffset = (gint) adj->value;
   } else if (adj == glg->vadj) {
      dy = glg->yoffset - (gint) adj->value;
      glg->yoffset = (gint) adj->value;
   }

   if ((dx != 0 || dy != 0) && GTK_WIDGET_REALIZED(glg)) {
#ifdef G_OS_UNIX
      glXWaitX();                  /* This is important */
      glXMakeCurrent(glg->dpy, GDK_DRAWABLE_XID(widget->window), glg->cx);
#endif                             /* linux */
#ifdef G_OS_WIN32
      wglMakeCurrent(glg->hDC, glg->hRC);
#endif                             /* WINDOWS */

      /* Get Window Sizes */
      x = (GLdouble) glg->hadj->value;
      y = (GLdouble) (((gdouble) glg->height) - glg->vadj->value);
      w = (GLdouble) widget->allocation.width;
      h = (GLdouble) widget->allocation.height;
      /* Fix y for OpenGL coordinate system */
      y = y - h;

      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      /* Clamp the window to whats visible */
      gluOrtho2D(x, x + w, y, y + h);

      gdk_window_scroll(widget->window, dx, dy);
   }
}

extern void gtk_glgraph_set_title(GtkGLGraph * glg, const gchar * const title)
{
   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));
   g_return_if_fail(title != NULL);
   g_return_if_fail(g_utf8_strlen(title, -1) != 0);

   /* Free the old title */
   if (glg->title) {
      g_free(glg->title);
   }

   /* Copy the new title */
   glg->title = g_strdup(title);

   /* Set the layout */
   if (GTK_WIDGET_REALIZED(GTK_WIDGET(glg)) == FALSE) {
      return;
   }

   pango_layout_set_markup(glg->title_layout, glg->title, -1);

   if (glg->title_pixbuf != NULL) {
      g_object_unref(glg->title_pixbuf);
      glg->title_pixbuf = NULL;
   }

   /* Create pixbuf */
   glg->title_pixbuf =
      gtk_glgraph_create_rotated_flipped_text_pixbuf(glg, glg->title_layout,
      GTKGLG_DIRECTION_0, FALSE, TRUE);
}

extern void gtk_glgraph_set_drawn(GtkGLGraph * glg, const GtkGLGDraw drawn)
{
   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   glg->drawn = drawn;
}

extern GtkGLGDraw gtk_glgraph_get_drawn(GtkGLGraph * glg)
{
   g_return_val_if_fail(glg != NULL, 0);
   g_return_val_if_fail(GTK_IS_GLGRAPH(glg), 0);

   return glg->drawn;
}

extern void gtk_glgraph_set_zoom(GtkGLGraph * glg, const gdouble zoom)
{
   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));
   if (zoom <= 0.0) {
      return;
   }

   glg->zoom = zoom;
   gtk_widget_queue_resize(GTK_WIDGET(glg));
}

extern gdouble gtk_glgraph_get_zoom(GtkGLGraph * glg)
{
   g_return_val_if_fail(glg != NULL, 1.0);
   g_return_val_if_fail(GTK_IS_GLGRAPH(glg), 1.0);

   return (glg->zoom);
}

static void gtk_glgraph_set_color(const GtkGLGraph * const glg, gdouble z,
   gdouble alpha, GtkGLGraphColormap cmap, gdouble ma, gdouble mi)
{
   gdouble h, i, q, t;
   gdouble tmp;

   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   /* Scale z from 0 to 1 */
   /* Protect from divide by 0 */
   tmp = ma - mi;
   tmp = tmp <= 0.0 ? 0.001 : tmp;
   z = (z - mi) / tmp;
   z = CLAMP(z, 0.0, 0.9999999);

   switch (cmap) {
   case GTKGLG_COLORMAP_GRAYSCALE:
      /* Set the color */
      glColor4f(z, z, z, alpha);
      break;
   case GTKGLG_COLORMAP_REDSCALE:
      h = z * 2.0;
      i = floor(h);
      q = h - i;

      switch (((gint) i)) {
      case 0:                     /* black to red */
         glColor4f(q, 0.0, 0.0, alpha);
         break;
      case 1:                     /* red to white */
      default:
         glColor4f(1.0, q, q, alpha);
         break;
      }
      break;
   case GTKGLG_COLORMAP_GREENSCALE:
      h = z * 2.0;
      i = floor(h);
      q = h - i;

      switch (((gint) i)) {
      case 0:                     /* black to green */
         glColor4f(0.0, q, 0.0, alpha);
         break;
      case 1:                     /* green to white */
      default:
         glColor4f(q, 1.0, q, alpha);
         break;
      }
      break;
   case GTKGLG_COLORMAP_BLUESCALE:
      h = z * 2.0;
      i = floor(h);
      q = h - i;

      switch (((gint) i)) {
      case 0:                     /* black to blue */
         glColor4f(0.0, 0.0, q, alpha);
         break;
      case 1:                     /* blue to white */
      default:
         glColor4f(q, q, 1.0, alpha);
         break;
      }
      break;
   case GTKGLG_COLORMAP_HSV:
      h = z * 6.0;                 /* 0.0 to 6.0 */
      i = floor(h);
      q = 1.0 - h + i;
      t = h - i;

      switch (((gint) i)) {
      case 0:
         glColor4f(1.0, t, 0.0, alpha);
         break;
      case 1:
         glColor4f(q, 1.0, 0.0, alpha);
         break;
      case 2:
         glColor4f(0.0, 1.0, t, alpha);
         break;
      case 3:
         glColor4f(0.0, q, 1.0, alpha);
         break;
      case 4:
         glColor4f(t, 0.0, 1.0, alpha);
         break;
      case 5:
      default:
         glColor4f(1.0, 0.0, q, alpha);
         break;
      }
      break;
   case GTKGLG_COLORMAP_JET:
      h = z * 3.0;                 /* 0.0 to 6.0 */
      i = floor(h);
      q = 1.0 - h + i;
      t = h - i;

      switch (((gint) i)) {
      case 0:
         glColor4f(0.0, t, 1.0, alpha);
         break;
      case 1:
         glColor4f(t, 1.0, q, alpha);
         break;
      case 2:
      default:
         glColor4f(1.0, q, 0.0, alpha);
         break;
      }
      break;
   default:
      break;
   }
}

extern GtkGLGDataSet *gtk_glgraph_dataset_create(void)
{
   GtkGLGDataSet *glds;

   glds = g_new0(GtkGLGDataSet, 1);

   glds->graph_type = GTKGLG_TYPE_XY;

   glds->x = NULL;
   glds->y = NULL;
   glds->z = NULL;
   glds->x_length = 0;
   glds->y_length = 0;
   glds->z_length = 0;
   glds->xi = NULL;
   glds->yi = NULL;
   glds->zi = NULL;
   glds->xi_length = 0;
   glds->yi_length = 0;

   glds->x_units = NULL;
   glds->y_units = NULL;
   glds->z_units = NULL;

   glds->line_color[0] = 1.0;
   glds->line_color[1] = 0.0;
   glds->line_color[2] = 0.0;
   glds->line_color[3] = 1.0;
   glds->line_color_name = g_strdup("black");
   glds->point_color[0] = 0.0;
   glds->point_color[1] = 1.0;
   glds->point_color[2] = 0.0;
   glds->point_color[3] = 1.0;
   glds->glgcmap = GTKGLG_COLORMAP_JET;
   glds->glgcmap_alpha = 0.0;

   glds->draw_points = TRUE;
   glds->draw_lines = TRUE;
   glds->stipple_line = FALSE;

   glds->line_width = 1.0;
   glds->line_stipple = 0xFFFF;
   glds->point_size = 3.0;

   glds->title_pixbuf = NULL;

   return glds;
}

extern void gtk_glgraph_dataset_free(GtkGLGDataSet ** glds)
{
   if (glds == NULL) {
      return;
   }
   if (*glds == NULL) {
      return;
   }

   g_free((*glds)->x);
   g_free((*glds)->y);
   g_free((*glds)->z);
   g_free((*glds)->xi);
   g_free((*glds)->yi);
   g_free((*glds)->zi);
   g_free((*glds)->x_units);
   g_free((*glds)->y_units);
   g_free((*glds)->z_units);
   if ((*glds)->title_pixbuf != NULL) {
      g_object_unref((*glds)->title_pixbuf);
   }

   g_free(*glds);

   *glds = NULL;
}

extern void gtk_glgraph_dataset_free_all(GtkGLGraph * const glg)
{
   GtkGLGDataSet *glds;
   GList *ld;

   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   if (glg == NULL) {
      return;
   }
   if (glg->datasets == NULL) {
      return;
   }

   ld = g_list_first(glg->datasets);
   while (ld != NULL) {
      glds = ld->data;
      ld = ld->next;

      gtk_glgraph_dataset_free(&glds);
   }
   g_list_free(glg->datasets);
   glg->datasets = NULL;
}

extern void gtk_glgraph_dataset_add(GtkGLGraph * const glg,
   GtkGLGDataSet * const glds)
{
   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));
   if (glg == NULL) {
      return;
   }
   if (glds == NULL) {
      return;
   }

   glg->datasets = g_list_append(glg->datasets, glds);
}

extern void gtk_glgraph_dataset_set_title(GtkGLGraph * const glg,
   GtkGLGDataSet * const glds, const gchar * const title)
{
   GtkWidget *widget;
   PangoLayout *layout;

   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));
   g_return_if_fail(glds != NULL);

   if (glds->title_pixbuf != NULL) {
      g_object_unref(G_OBJECT(glds->title_pixbuf));
   }

   /* Create the layout */
   widget = GTK_WIDGET(glg);
   if (!GTK_WIDGET_REALIZED(widget)) {
      gtk_widget_realize(widget);
   }
   layout = gtk_widget_create_pango_layout(widget, NULL);
   pango_layout_set_markup(layout, title, -1);
   glds->title_pixbuf =
      gtk_glgraph_create_rotated_flipped_text_pixbuf(glg, layout,
      GTKGLG_DIRECTION_0, FALSE, TRUE);
   g_object_unref(G_OBJECT(layout));
}

extern void gtk_glgraph_dataset_linear_interpolate(GtkGLGDataSet * const glds,
   gint factor)
{
   gdouble *xi, *yi, *zi, *z;
   gdouble c[4], d[2];
   gint nw, nh, ow, oh, extra;
   gdouble start, spacing, inc;
   gint x, xone, xtwo;
   gint y, yone, ytwo;
   gint i, istart, istop;
   gint j, jstart, jstop;
   gint index_a, index_b, index_c;

   if (glds == NULL) {
      return;
   }

   /* Free old interpolated data */
   g_free(glds->xi);
   g_free(glds->yi);
   g_free(glds->zi);
   glds->xi = NULL;
   glds->yi = NULL;
   glds->zi = NULL;

   /* Make factor odd */
   factor |= 0x01;
   extra = (factor - 1) / 2;

   /* Set new and old dimensions */
   ow = glds->x_length;
   nw = ow * factor;
   glds->xi_length = nw;

   oh = glds->y_length;
   nh = oh * factor;
   glds->yi_length = nh;

   /* Fetch memory for the arrays */
   zi = g_malloc(sizeof(gdouble) * nw * nh);
   xi = g_malloc(sizeof(gdouble) * nw);
   yi = g_malloc(sizeof(gdouble) * nh);
   z = glds->z;

   /* Set the data to zero */
   for (i = 0; i < nw * nh; i++) {
      zi[i] = 0.0;
   }

   /* Set up new X spacing */
   spacing = glds->x[1] - glds->x[0];
   inc = spacing / ((gdouble) factor);
   start = glds->x[0] - inc * ((gdouble) extra);
   for (i = 1, xi[0] = start; i < nw; i++) {
      xi[i] = xi[i - 1] + inc;
   }

   /* Set up new Y spacing */
   spacing = glds->y[1] - glds->y[0];
   inc = spacing / ((gdouble) factor);
   start = glds->y[0] - inc * ((gdouble) extra);
   for (i = 1, yi[0] = start; i < nh; i++) {
      yi[i] = yi[i - 1] + inc;
   }

   /* Interpolate manually by factor */
   for (y = -1; y < oh; y++) {
      /* get y values for the four corners */
      yone = CLAMP(y, 0, oh - 1);
      ytwo = CLAMP(y + 1, 0, oh - 1);

      /* Fix scales for j loop */
      jstart = (yone != y) ? factor - extra : 0;
      jstop = (ytwo != y + 1) ? extra : factor;

      /* Start final index calc */
      index_a = y * nw * factor + nw * extra + extra;

      for (x = -1; x < ow; x++) {
         /* get x values for the four corners */
         xone = CLAMP(x, 0, ow - 1);
         xtwo = CLAMP(x + 1, 0, ow - 1);

         /* Fix scales for i loop */
         istart = (xone != x) ? factor - extra : 0;
         istop = (xtwo != x + 1) ? extra : factor;

         /* Set the four corners */
         c[0] = z[yone * ow + xone];
         c[1] = z[ytwo * ow + xone];
         c[2] = z[yone * ow + xtwo];
         c[3] = z[ytwo * ow + xtwo];

         /* Add to final index calc */
         index_b = index_a + x * factor;

         /* Fill in the values in the interpolated array */
         for (j = jstart; j <= jstop; j++) {

            d[0] = (((gdouble) (factor - j)) * c[0] +
               ((gdouble) j) * c[1]) / ((gdouble) factor);
            d[1] = (((gdouble) (factor - j)) * c[2] +
               ((gdouble) j) * c[3]) / ((gdouble) factor);

            /* Add to final index calc */
            index_c = index_b + j * nw;

            for (i = istart; i <= istop; i++) {
               zi[index_c + i] =
                  (((gdouble) (factor - i)) * d[0] +
                  ((gdouble) i) * d[1]) / ((gdouble) factor);
            }
         }
      }
   }

   /* Remember new values */
   glds->xi = xi;
   glds->yi = yi;
   glds->zi = zi;
}

extern void gtk_glgraph_dataset_bicubic_interpolate(GtkGLGDataSet * const glds,
   gint factor)
{
   gdouble *z, *zout;
   gint i, j, il, jl, jln, iln, ilm, jlm;
   gdouble a, b, da, db, sub;
   gdouble x, y;
   gint m, n, c, d;
   gint index;
   gdouble w;
   FILE *f;

   if (glds == NULL) {
      return;
   }
   if (glds->z == NULL) {
      return;
   }
   g_free(glds->zi);
   glds->zi = NULL;
   /* glds->zi_subdivisions = subdivisions;  */

   il = glds->x_length;
   jl = glds->y_length;
   ilm = il - 1;
   jlm = jl - 1;
   iln = il * factor;
   jln = jl * factor;
   sub = (gdouble) factor;

   z = glds->z;

   /* Allocate memory for the output data */
   zout = g_malloc(sizeof(gdouble) * il * jl * factor * factor);

   f = fopen("/tmp/data.tst", "w");

   /* Interpolate */
   for (j = 0; j < jln; j++) {
      for (i = 0; i < iln; i++) {
         a = ((gdouble) i) / sub;
         b = ((gdouble) j) / sub;
         da = a - floor(a);
         db = b - floor(b);
         c = (gint) floor(a);
         d = (gint) floor(b);
         zout[j * iln + i] = 0;

         for (n = -1; n <= 2; n++) {
            for (m = -1; m <= 2; m++) {
               x = CLAMP(c + m, 0, ilm);
               y = CLAMP(d + n, 0, jlm);

               index = y * il + x;
               w = BiCubicR(((gdouble) m) - da) * BiCubicR(((gdouble) n) - db);

               zout[j * iln + i] += w * z[index];
            }
         }
         fprintf(f, "%0.3f ", zout[j * iln + i]);
      }
      fprintf(f, "\n");
   }

   glds->zi = zout;

   fclose(f);
}

static gdouble BiCubicR(gdouble x)
{
   gdouble ret = 0;

   x += 2.0;
   if (x > 0.0) {
      ret += x * x * x;
   }

   x -= 1.0;
   if (x > 0.0) {
      ret -= 4 * x * x * x;
   }

   x -= 1.0;
   if (x > 0.0) {
      ret += 6 * x * x * x;
   }

   x -= 1.0;
   if (x > 0.0) {
      ret -= 4 * x * x * x;
   }

   ret /= 6.0;

   return ret;
}

extern void gtk_glgraph_redraw(GtkGLGraph * const glg)
{
   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   if (glg->queued_draw >= 1) {
      return;
   }

   glg->queued_draw = 1;

   gtk_widget_queue_draw(GTK_WIDGET(glg));
}

static void gtk_glgraph_draw_gl_background(const GtkGLGraph * glg)
{
   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   /* Set the clear color */
   glClearColor(1.0, 1.0, 1.0, 0.0);
   /* Clear the Graph */
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

static void gtk_glgraph_draw_graph_box(const GtkGLGraph * glg)
{
   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   /* Set Colour to Black */
   glColor3f(0.0, 0.0, 0.0);
   glLineWidth(2.0);

   /* Draw the Axes */
   glBegin(GL_LINE_LOOP);
   glVertex2d(glg->left, glg->bottom);
   glVertex2d(glg->w - glg->right, glg->bottom);
   glVertex2d(glg->w - glg->right, glg->h - glg->top);
   glVertex2d(glg->left, glg->h - glg->top);
   glEnd();
}

static void gtk_glgraph_draw_tooltip(GtkGLGraph * const glg,
   const GtkGLGDataSet * const glds)
{
   gdouble xpo, xppu, ypo, yppu;   /* x pixel offset, x pixels per unit, etc... */
   gdouble ax, ay;                 /* x array index, and y data-value */
   gint ix, iy;
   gdouble xspacing, yspacing;
   gchar *buf;
   gint pw, ph;
   gdouble x, y;
   gdouble xc, yc;
   const guchar *pixels;
   gdouble xpos, ypos;
   gdouble xmin, xmax;
   gdouble ttx, tty;
   GtkGLGDataSet *gdata = NULL;
   GList *find_ds = NULL;

   /* Make sure all the data we need exists */
   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));
   if (glds == NULL) {
      return;
   }
   if (glds->x == NULL || glds->y == NULL || glds->z == NULL) {
      return;
   }
   if (glds->x_length < 2 || glds->y_length < 2) {
      return;
   }
   if ((glg->drawn & GTKGLG_D_TOOLTIP) == 0) {
      return;
   }
   if (glg->tooltip_visible == FALSE) {
      return;
   }

   /* Get Scaling */
   xppu = fabs((glg->w - glg->left - glg->right) /
      (glg->axes[GTKGLG_AXIS_X].max - glg->axes[GTKGLG_AXIS_X].min));
   yppu = fabs((glg->h - glg->bottom - glg->top) /
      (glg->axes[GTKGLG_AXIS_Y].max - glg->axes[GTKGLG_AXIS_Y].min));
   xpo = glg->left - glg->axes[GTKGLG_AXIS_X].min * xppu;
   ypo = glg->bottom - glg->axes[GTKGLG_AXIS_Y].min * yppu;

   /* remap ttx and tty from local window to global window and pixels to inches */
   ttx = glg->ttx + glg->hadj->value;
   tty = glg->tty + glg->vadj->value;
   ax = (ttx - xpo) / xppu;
   ay = (glg->h - tty - ypo) / yppu;

   /* If cursor isn't in the graph, return */
   if (ax < glg->axes[GTKGLG_AXIS_X].min || ax > glg->axes[GTKGLG_AXIS_X].max) {
      return;
   }
   if (ay < glg->axes[GTKGLG_AXIS_Y].min || ay > glg->axes[GTKGLG_AXIS_Y].max) {
      return;
   }

   /* Get spacing */
   xspacing = glds->x[1] - glds->x[0];
   yspacing = glds->y[1] - glds->y[0];

   /* Figure out the dataset indices */
   /* And create a pixbuf of the x,y value */
   ix = (gint) floor((ax - glg->axes[GTKGLG_AXIS_X].min) / xspacing) + 1;
   iy = (gint) ix;

/* *** */
   find_ds = g_list_first(glg->datasets);
   while (find_ds) {
      gdata = find_ds->data;
      find_ds = find_ds->next;
      if ((gdata->y[ix] >= (ay * 0.96)) && (gdata->y[ix] <= (ay * 1.04))) {
         break;                    /* found it (y value) */
      }
      gdata = NULL;
   }

   if (gdata == NULL) {
      return;                      /* did not find the right one */
   }

   /* ay is y's value, ix+1 is x's value */

   if (gdata->graph_type == GTKGLG_TYPE_XY) {
      buf = g_strdup_printf("%3.1lf%% "
         "<span foreground=\"%s\">"
         "<b>%s</b></span>\n"
         "%2.0d <i>%s</i>",
         gdata->y[iy], gdata->line_color_name, gdata->y_units, ix, gdata->x_units);
   } else {                        /* dont care now */
      iy = (gint) floor((ay - glg->axes[GTKGLG_AXIS_Y].min) / yspacing);
      buf = g_strdup_printf("%01.3f <b>in.</b>", glds->z[iy * glds->x_length + ix]);
   }

   gtk_glgraph_pixbuf_update_tooltip(glg, buf);
   g_free(buf);

   /* Draw Arrowhead -- WHY the pointer is already overing this arrowhead spot */

/*  
  glColor4f (0.0, 0.0, 0.0, 1.0);
  glBegin (GL_TRIANGLES);
  glVertex2d ((gdata->x[ix]) * xppu + xpo, (gdata->y[iy]) * yppu + ypo);
  glVertex2d ((gdata->x[ix] + 0.04) * xppu + xpo, (gdata->y[iy] + 0.150) * yppu + ypo);
  glVertex2d ((gdata->x[ix] + 0.150) * xppu + xpo, (gdata->y[iy] + 0.04) * yppu + ypo);
  glEnd (); 
*/
   /* Figure out quadrant and draw appropriately */
   if (glg->tooltip_pixbuf == NULL) {
      return;
   }

   pw = gdk_pixbuf_get_width(glg->tooltip_pixbuf);
   ph = gdk_pixbuf_get_height(glg->tooltip_pixbuf);

   /* Determine Side */
   /* Find on screen max and min x */
   xmin = MAX(glg->hadj->value, glg->left);
   xmax = MIN(glg->hadj->value + glg->hadj->page_size, glg->w - glg->right);

   if (ttx < 0.5 * (xmax + xmin)) {
      /* Draw on right */
      xpos = MIN(glg->w - glg->right, xmax) - INSET - TTX_BORDER - pw;
   } else {
      /* Draw on left */
      xpos = MAX(glg->left, xmin) + INSET;
   }
   ypos = glg->h - MAX(glg->vadj->value, glg->top) - INSET - TTY_BORDER - ph;

   glEnable(GL_BLEND);
   glLineWidth(1.0);

   /* Draw Black Shadow Box */
   glColor4f(0.0, 0.0, 0.0, 0.5);
   glBegin(GL_QUADS);
   glVertex2d(xpos + 2.0, ypos - 2.0 + ph + TTY_BORDER);
   glVertex2d(xpos + 2.0 + pw + TTX_BORDER, ypos - 2.0 + ph + TTY_BORDER);
   glVertex2d(xpos + 2.0 + pw + TTX_BORDER, ypos - 2.0);
   glVertex2d(xpos + 2.0, ypos - 2.0);
   glEnd();
   glDisable(GL_BLEND);

   /* Draw Black Box */
   glColor4f(0.0, 0.0, 0.0, 1.0);
   glBegin(GL_QUADS);
   glVertex2d(xpos, ypos + ph + TTY_BORDER);
   glVertex2d(xpos + pw + TTX_BORDER, ypos + ph + TTY_BORDER);
   glVertex2d(xpos + pw + TTX_BORDER, ypos);
   glVertex2d(xpos, ypos);
   glEnd();

   /* Draw White Box */
   glColor4f(1.0, 1.0, 1.0, 1.0);
   glBegin(GL_QUADS);
   glVertex2d(xpos + 1.0, ypos - 1.0 + ph + TTY_BORDER);
   glVertex2d(xpos - 1.0 + pw + TTX_BORDER, ypos - 1.0 + ph + TTY_BORDER);
   glVertex2d(xpos - 1.0 + pw + TTX_BORDER, ypos + 1.0);
   glVertex2d(xpos + 1.0, ypos + 1.0);
   glEnd();

   /* Draw Text */
   xc = glg->hadj->value;
   yc = glg->vadj->upper - glg->vadj->value - glg->vadj->page_size;

   x = xpos + TTX_BORDER * 0.5;
   y = ypos + TTY_BORDER * 0.5;

   if (x < xc || y < yc) {
      /* Raster pos. would be invalid */
      glRasterPos2d(xc + 0.01, yc + 0.01);      /* 0.01 prevents rounding error */
      glBitmap(0, 0, 0, 0, x - xc, y - yc, NULL);
   } else {
      glRasterPos2d(x, y);
   }

   pixels = gdk_pixbuf_get_pixels(glg->tooltip_pixbuf);
   glDrawPixels(pw, ph, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid *) pixels);

}

static void gtk_glgraph_draw_dataset(const GtkGLGraph * const glg,
   const GtkGLGDataSet * const glds)
{
   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));
   if (glds == NULL) {
      return;
   }

   switch (glds->graph_type) {
   case GTKGLG_TYPE_XY:
      gtk_glgraph_draw_dataset_xy(glg, glds);
      break;
   case GTKGLG_TYPE_SURFACE:
   case GTKGLG_TYPE_SURFACE_BICUBIC_INTERPOLATION:
      gtk_glgraph_draw_dataset_surface(glg, glds);
      break;
   default:
      break;
   }
}

static void gtk_glgraph_draw_dataset_xy(const GtkGLGraph * const glg,
   const GtkGLGDataSet * const glds)
{
   gdouble *x, *y;
   gdouble xpo, xppu, ypo, yppu;   /* x pixel offset, x pixels per unit, etc... */
   gdouble *color;
   gint count;

   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   if (glds->x == NULL || glds->y == NULL) {
      return;
   }

   x = glds->x;
   y = glds->y;

   xpo = glg->left;
   ypo = glg->bottom;
   xppu = fabs((glg->w - glg->left - glg->right) / (glg->axes[GTKGLG_AXIS_X].max -
         glg->axes[GTKGLG_AXIS_X].min));
   yppu = fabs((glg->h - glg->bottom - glg->top) / (glg->axes[GTKGLG_AXIS_Y].max -
         glg->axes[GTKGLG_AXIS_Y].min));

   /* Draw the lines */
   if (glds->draw_lines) {
      color = (gdouble *) (glds->line_color);
      glColor4f(color[0], color[1], color[2], color[3]);

      glLineWidth(glds->line_width);
      if (glds->stipple_line) {
         glLineStipple(1, glds->line_stipple);
         glEnable(GL_LINE_STIPPLE);
      }
      /* Really draw the lines */
      glBegin(GL_LINE_STRIP);
      for (count = 0; count < glds->x_length; count++) {
         glVertex2d((x[count] - glg->axes[GTKGLG_AXIS_X].min) * xppu + xpo,
            (y[count] - glg->axes[GTKGLG_AXIS_Y].min) * yppu + ypo);
      }
      glEnd();
      if (glds->stipple_line) {
         glDisable(GL_LINE_STIPPLE);
      }
   }

   /* Draw the points */
   if (glds->draw_points) {
      GLUquadricObj *quadric;

      color = (gdouble *) (glds->point_color);
      glColor4f(color[0], color[1], color[2], color[3]);

      quadric = gluNewQuadric();
      gluQuadricDrawStyle(quadric, GLU_FILL);

      /* Really draw the points */
      for (count = 0; count < glds->x_length; count++) {
         glPushMatrix();
         glTranslated((x[count] - glg->axes[GTKGLG_AXIS_X].min) * xppu + xpo,
            (y[count] - glg->axes[GTKGLG_AXIS_Y].min) * yppu + ypo, 0.0);
         gluDisk(quadric, 0.0, glds->point_size * 0.5, 15, 2);
         glPopMatrix();
      }
      gluDeleteQuadric(quadric);
   }
}

static void gtk_glgraph_draw_dataset_surface(const GtkGLGraph * const glg,
   const GtkGLGDataSet * const glds)
{
   gdouble *x, *y, *z;
   gdouble xpo, xppu, ypo, yppu;   /* x pixel offset, x pixels per unit, etc... */
   gint count, count2;
   gdouble ma, mi;
   gint xl, yl;
   gdouble pixel_size;
   gdouble tempp, tempm;

   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   if (glds->zi == NULL || glds->xi == NULL || glds->yi == NULL ||
      glds->xi_length <= 0 || glds->yi_length <= 0) {

      x = glds->x;
      y = glds->y;
      z = glds->z;
      xl = glds->x_length;
      yl = glds->y_length;

      if (x == NULL || y == NULL || z == NULL) {
         return;
      }
   } else {
      x = glds->xi;
      y = glds->yi;
      z = glds->zi;
      xl = glds->xi_length;
      yl = glds->yi_length;
   }

   xpo = glg->left;
   ypo = glg->bottom;
   xppu = fabs((glg->w - glg->left - glg->right) / (glg->axes[GTKGLG_AXIS_X].max -
         glg->axes[GTKGLG_AXIS_X].min));
   yppu = fabs((glg->h - glg->bottom - glg->top) / (glg->axes[GTKGLG_AXIS_Y].max -
         glg->axes[GTKGLG_AXIS_Y].min));

   ma = glg->axes[GTKGLG_AXIS_Z].max;
   mi = glg->axes[GTKGLG_AXIS_Z].min;

   glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

   pixel_size = (x[1] - x[0]) * 0.5;

   /* Draw with OpenGL interpolation */
   /*for (count = 0; count < yl - 1; count++) {
    * glBegin (GL_QUAD_STRIP);
    * for (count2 = 0; count2 < xl; count2++) { */
   /* The Point */
   /*  gtk_glgraph_set_color (glg, *(z), glds->glgcmap_alpha, glds->glgcmap, ma, mi);
    * glVertex2d (x[count2] * xppu + xpo, y[count] * yppu + ypo); */
   /* One below */
   /*  gtk_glgraph_set_color (glg, *(z + xl), glds->glgcmap_alpha, glds->glgcmap, ma, mi);
    * glVertex2d (x[count2] * xppu + xpo, y[count + 1] * yppu + ypo);
    * z++;
    * }
    * glEnd ();
    * } */

   /* Draw rectangles */
   glBegin(GL_QUADS);
   for (count = 0; count < yl; count++) {
      tempp = (y[count] + pixel_size) * yppu + ypo;
      tempm = (y[count] - pixel_size) * yppu + ypo;

      for (count2 = 0; count2 < xl; count2++) {
         gtk_glgraph_set_color(glg, *(z), glds->glgcmap_alpha, glds->glgcmap, ma,
            mi);

         glVertex2d((x[count2] + pixel_size) * xppu + xpo, tempp);
         glVertex2d((x[count2] + pixel_size) * xppu + xpo, tempm);
         glVertex2d((x[count2] - pixel_size) * xppu + xpo, tempm);
         glVertex2d((x[count2] - pixel_size) * xppu + xpo, tempp);
         z++;
      }
   }
   glEnd();
}

/* Pango Layout Stuff */
static void gtk_glgraph_draw_title(const GtkGLGraph * glg)
{
   GtkGLGDraw drawn;
   gdouble y, x;
   gint width, height;
   const guchar *pixels;
   gdouble xc, yc;

   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   drawn = glg->drawn;
   if (!(drawn & GTKGLG_D_TITLE)) {
      return;
   }
   if (glg->title_pixbuf == NULL) {
      return;
   }

   /* Figure out whats known */
   height = gdk_pixbuf_get_height(glg->title_pixbuf);
   width = gdk_pixbuf_get_width(glg->title_pixbuf);

   /* Find the left point */
   x = (glg->w + glg->left - glg->right - (gdouble) width) / 2.0;
   y = glg->h - GTKGLG_TEXT - height;

   /* Get the Clipping Area */
   /* Stupid hack because openGL requires RasterPos to be in the viewport */
   xc = glg->hadj->value;
   yc = glg->vadj->upper - glg->vadj->value - glg->vadj->page_size;

   if (x < xc || y < yc) {
      /* Raster pos. would be invalid */
      glRasterPos2d(xc + 0.01, yc + 0.01);      /* 0.01 prevents rounding error */
      glBitmap(0, 0, 0, 0, x - xc, y - yc, NULL);
   } else {
      glRasterPos2d(x, y);
   }

   /* Draw the x axis title */
   pixels = gdk_pixbuf_get_pixels(glg->title_pixbuf);
   glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid *) pixels);
}

static void gtk_glgraph_draw_legend(const GtkGLGraph * glg)
{
   gdouble x1, x2;
   gdouble y1, y2, ym;
   gdouble l1, l2, p;
   GList *datasets;
   GtkGLGDataSet *dataset;
   gdouble *color;
   gdouble ph;
   gint width, height, dscount = 0;
   const guchar *pixels;
   gdouble xc, yc;
   gdouble xp;
   GtkGLGDraw drawn;
   gboolean b_flag = FALSE;

   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   drawn = glg->drawn;
   if (!(drawn & GTKGLG_D_LEGEND)) {
      return;
   }

   /* Define the Legend corners */
   if (glg->legend_in_out == TRUE) {
      x1 = (((glg->w) / 5) * 4) - glg->legend_text_width;
      x2 = (((glg->w) / 5) * 4) + (3 * LEGEND_MARGIN + LEGEND_LINE_LENGTH);
      y1 = glg->h - (((glg->top) / 5) * 6);
      y2 = glg->h - (((glg->top) / 5) * 6);
   } else {
      x1 = glg->w - glg->l_left;
      x2 = glg->w - BORDER;
      y1 = glg->h - glg->top;
      y2 = glg->h - glg->top;
   }

   /* Define the lines and point */
   l1 = x1 + LEGEND_MARGIN;
   l2 = l1 + LEGEND_LINE_LENGTH;
   p = (l1 + l2) * 0.5;
   xp = l2 + LEGEND_MARGIN;

   dscount = g_list_length(glg->datasets);
   datasets = g_list_first(glg->datasets);
   while (datasets) {
      dataset = datasets->data;
      datasets = datasets->next;

      /* Draw if the title exists */
      if (dataset->title_pixbuf != NULL) {

         y2 -= LEGEND_MARGIN;
         width = gdk_pixbuf_get_width(dataset->title_pixbuf);
         height = gdk_pixbuf_get_height(dataset->title_pixbuf);
         ph = (gdouble) height;

         if (!b_flag) {
            dscount++;
            ym = y2 - (dscount * ph);
            /* Set Colour to white then black */
            glColor4f(1.0, 1.0, 1.0, 1.0);
            glLineWidth(2.0);

            /* Draw the legend border */
            glBegin(GL_QUADS);
            glVertex2d(x1, ym);
            glVertex2d(x2, ym);
            glVertex2d(x2, y1);
            glVertex2d(x1, y1);
            glEnd();
            glColor4f(0.0, 0.0, 0.0, 1.0);
            glBegin(GL_LINE_LOOP);
            glVertex2d(x1, ym);
            glVertex2d(x2, ym);
            glVertex2d(x2, y1);
            glVertex2d(x1, y1);
            glEnd();

            b_flag = TRUE;         /* one shot flag */
         }

         /* Draw the line */
         if (dataset->draw_lines) {
            color = (gdouble *) (dataset->line_color);
            glColor4f(color[0], color[1], color[2], color[3]);

            glLineWidth(dataset->line_width);
            if (dataset->stipple_line) {
               glLineStipple(1, dataset->line_stipple);
               glEnable(GL_LINE_STIPPLE);
            }
            /* Really draw the lines */
            glBegin(GL_LINE_STRIP);
            glVertex2d(l1, y2 - 0.5 * ph);
            glVertex2d(l2, y2 - 0.5 * ph);
            glEnd();
            if (dataset->stipple_line) {
               glDisable(GL_LINE_STIPPLE);
            }
         }

         /* Draw the point */
         if (dataset->draw_points) {
            GLUquadricObj *quadric;

            color = (gdouble *) (dataset->point_color);
            glColor4f(color[0], color[1], color[2], color[3]);

            quadric = gluNewQuadric();
            gluQuadricDrawStyle(quadric, GLU_FILL);

            /* Really draw the point */
            glPushMatrix();
            glTranslated(p, y2 - 0.5 * ph, 0.0);
            gluDisk(quadric, 0.0, dataset->point_size * 0.5, 15, 2);
            glPopMatrix();

            gluDeleteQuadric(quadric);
         }

         y2 -= ph;

         /* Get the Clipping Area */
         /* Stupid hack because openGL requires RasterPos to be in the viewport */
         xc = glg->hadj->value;
         yc = glg->vadj->upper - glg->vadj->value - glg->vadj->page_size;

         if (xp < xc || y2 < yc) {
            /* Raster pos. would be invalid */
            glRasterPos2d(xc + 0.01, yc + 0.01);        /* 0.01 prevents rounding error */
            glBitmap(0, 0, 0, 0, xp - xc, y2 - yc, NULL);
         } else {
            glRasterPos2d(xp, y2);
         }

         /* Draw the dataset title */
         pixels = gdk_pixbuf_get_pixels(dataset->title_pixbuf);
         glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid *) pixels);
      }
   }

}

static void gtk_glgraph_draw_cursor(const GtkGLGraph * glg)
{
   GtkGLGDraw drawn;

   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   drawn = glg->drawn;

   /* Draw what needs to be drawn */
   if (drawn & GTKGLG_D_CURSOR_BOX) {
      gtk_glgraph_draw_cursor_boxes(glg);
   }
   if (drawn & GTKGLG_D_HORIZONTAL_CURSOR) {
      gtk_glgraph_draw_cursor_horizontal(glg);
   }
   if (drawn & GTKGLG_D_VERTICAL_CURSOR) {
      gtk_glgraph_draw_cursor_vertical(glg);
   }
}

static void gtk_glgraph_draw_cursor_boxes(const GtkGLGraph * glg)
{
   gdouble xpo, xppu, ypo, yppu;   /* x pixel offset, x pixels per unit, etc... */

   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   xpo = glg->left;
   ypo = glg->bottom;
   xppu = fabs((glg->w - glg->left - glg->right) / (glg->axes[GTKGLG_AXIS_X].max -
         glg->axes[GTKGLG_AXIS_X].min));
   yppu = fabs((glg->h - glg->bottom - glg->top) / (glg->axes[GTKGLG_AXIS_Y].max -
         glg->axes[GTKGLG_AXIS_Y].min));

   /* draw a white box */
   glColor3f(1.0, 1.0, 1.0);
   glBegin(GL_LINE_LOOP);
   glVertex2d(glg->cx1 * xppu + xpo, glg->cy1 * yppu + ypo);
   glVertex2d(glg->cx2 * xppu + xpo, glg->cy1 * yppu + ypo);
   glVertex2d(glg->cx2 * xppu + xpo, glg->cy2 * yppu + ypo);
   glVertex2d(glg->cx1 * xppu + xpo, glg->cy2 * yppu + ypo);
   glEnd();

   /* draw a black box with stipple */
   /* draw a white box */
   glColor3f(0.0, 0.0, 0.0);
   glLineStipple(1, glg->cursor_stipple);
   glEnable(GL_LINE_STIPPLE);
   glBegin(GL_LINE_LOOP);
   glVertex2d(glg->cx1 * xppu + xpo, glg->cy1 * yppu + ypo);
   glVertex2d(glg->cx2 * xppu + xpo, glg->cy1 * yppu + ypo);
   glVertex2d(glg->cx2 * xppu + xpo, glg->cy2 * yppu + ypo);
   glVertex2d(glg->cx1 * xppu + xpo, glg->cy2 * yppu + ypo);
   glEnd();
   glDisable(GL_LINE_STIPPLE);
}

static void gtk_glgraph_draw_cursor_horizontal(const GtkGLGraph * glg)
{
   /* if (glg->hcursors == NULL) { return; } */
}

static void gtk_glgraph_draw_cursor_vertical(const GtkGLGraph * glg)
{
   /* if (glg->vcursors == NULL) { return; }  */
}

static void gtk_glgraph_draw_x_axis(const GtkGLGraph * glg)
{
   GtkGLGDrawAxis drawn;

   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   drawn = glg->axes[GTKGLG_AXIS_X].drawn;

   if (!(drawn & GTKGLG_DA_AXIS)) {
      return;
   }

   /* Set Colour to Black */
   glColor3f(0.0, 0.0, 0.0);
   glLineWidth(1.0);

   /* Draw what needs to be drawn */
   if (drawn & GTKGLG_DA_INSIDE_TICK_MARKS) {
      gtk_glgraph_draw_x_inside_tick_marks(glg);
   }
   if (drawn & GTKGLG_DA_OUTSIDE_TICK_MARKS) {
      gtk_glgraph_draw_x_outside_tick_marks(glg);
   }
   if (drawn & GTKGLG_DA_MAJOR_GRID) {
      gtk_glgraph_draw_x_major_grid(glg);
   }
   if (drawn & GTKGLG_DA_MINOR_GRID) {
      gtk_glgraph_draw_x_minor_grid(glg);
   }
   if (drawn & GTKGLG_DA_TITLE) {
      gtk_glgraph_draw_x_title(glg);
   }
   if (drawn & GTKGLG_DA_MAJOR_TICK_TEXT) {
      gtk_glgraph_draw_x_major_tick_text(glg);
   }

}

static void gtk_glgraph_draw_x_inside_tick_marks(const GtkGLGraph * glg)
{
   gdouble inc;
   gdouble yt, yb, i;
   gint j, steps;

   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   /* Figure out whats known */
   yb = glg->bottom;
   yt = glg->h - glg->top;

   steps = glg->axes[GTKGLG_AXIS_X].major_steps;
   inc = (glg->w - glg->left - glg->right) / (gdouble) steps;

   /* Draw the horizontal inside ticks */
   glBegin(GL_LINES);
   for (i = glg->left, j = 0; j <= steps; i = i + inc, j++) {
      glVertex2d(i, yb);
      glVertex2d(i, yb + 5);
      glVertex2d(i, yt);
      glVertex2d(i, yt - 5);
   }
   glEnd();
}

static void gtk_glgraph_draw_x_outside_tick_marks(const GtkGLGraph * glg)
{
   gdouble inc;
   gdouble yt, yb, i;
   gint j, steps;

   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   /* Figure out whats known */
   yb = glg->bottom;
   yt = glg->h - glg->top;

   steps = glg->axes[GTKGLG_AXIS_X].major_steps;
   inc = (glg->w - glg->left - glg->right) / (gdouble) steps;

   /* Draw the horizontal outside ticks */
   glBegin(GL_LINES);
   for (i = glg->left, j = 0; j <= steps; i = i + inc, j++) {
      glVertex2d(i, yb);
      glVertex2d(i, yb - 5);
      glVertex2d(i, yt);
      glVertex2d(i, yt + 5);
   }
   glEnd();
}

static void gtk_glgraph_draw_x_major_grid(const GtkGLGraph * glg)
{
   gdouble inc;
   gdouble yt, yb, i;
   gint j, steps;

   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   /* Figure out whats known */
   yb = glg->bottom;
   yt = glg->h - glg->top;

   steps = glg->axes[GTKGLG_AXIS_X].major_steps;
   inc = (glg->w - glg->left - glg->right) / (gdouble) steps;
   glLineWidth(1.0);
   /* Draw the horizontal major grid */
   glBegin(GL_LINES);
   for (i = glg->left, j = 0; j <= steps; i = i + inc, j++) {
      glVertex2d(i, yb);
      glVertex2d(i, yt);
   }
   glEnd();
}

static void gtk_glgraph_draw_x_minor_grid(const GtkGLGraph * glg)
{
   gdouble inc;
   gdouble yt, yb, i, k;
   gint j, major_steps, minor_steps;

   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   /* Set Colour to Gray */
   glColor3f(0.7, 0.7, 0.7);

   /* Figure out whats known */
   yb = glg->bottom;
   yt = glg->h - glg->top;

   minor_steps = glg->axes[GTKGLG_AXIS_X].minor_steps;
   major_steps = glg->axes[GTKGLG_AXIS_X].major_steps;
   inc = (glg->w - glg->left -
      glg->right) / (gdouble) major_steps / (gdouble) minor_steps;

   /* Draw the horizontal minor grid */
   glBegin(GL_LINES);
   for (i = glg->left + inc, j = 0; j < major_steps; i = i + inc, j++) {
      for (k = 1; k < minor_steps; i = i + inc, k++) {
         glVertex2d(i, yb);
         glVertex2d(i, yt);
      }
   }
   glEnd();
}

static void gtk_glgraph_draw_x_title(const GtkGLGraph * glg)
{
   gdouble y, x;
   gint width, height;
   const guchar *pixels;
   gdouble xc, yc;

   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   if (glg->axes[GTKGLG_AXIS_X].pixbuf == NULL) {
      return;
   }

   /* Figure out whats known */
   height = gdk_pixbuf_get_height(glg->axes[GTKGLG_AXIS_X].pixbuf);
   width = gdk_pixbuf_get_width(glg->axes[GTKGLG_AXIS_X].pixbuf);

   /* Find the left point */
   x = (glg->w + glg->left - glg->right - (gdouble) width) / 2.0;
   y = GTKGLG_TEXT;

   /* Get the Clipping Area */
   /* Stupid hack because openGL requires RasterPos to be in the viewport */
   xc = glg->hadj->value;
   yc = glg->vadj->upper - glg->vadj->value - glg->vadj->page_size;

   if (x < xc || y < yc) {
      /* Raster pos. would be invalid */
      glRasterPos2d(xc + 0.01, yc + 0.01);      /* 0.01 prevents rounding error */
      glBitmap(0, 0, 0, 0, x - xc, y - yc, NULL);
   } else {
      glRasterPos2d(x, y);
   }

   /* Draw the x axis title */
   pixels = gdk_pixbuf_get_pixels(glg->axes[GTKGLG_AXIS_X].pixbuf);
   glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid *) pixels);
}

static void gtk_glgraph_draw_x_major_tick_text(const GtkGLGraph * glg)
{
   gint32 j, steps;
   GdkPixbuf *pixbuf;
   gint width, height;
   gdouble inc, i;
   gdouble x, y;
   const guchar *pixels;
   gdouble xc, yc;

   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   if (glg->axes[GTKGLG_AXIS_X].major_tick_pixbuf_list == NULL) {
      return;
   }

   /* Figure out whats known */
   y = glg->bottom - GTKGLG_TEXT - glg->axes[GTKGLG_AXIS_X].tick_height;

   steps = glg->axes[GTKGLG_AXIS_X].major_steps;
   inc = (glg->w - glg->left - glg->right) / (gdouble) steps;

   /* Get the Clipping Area */
   /* Stupid hack because openGL requires RasterPos to be in the viewport */
   xc = glg->hadj->value;
   yc = glg->vadj->upper - glg->vadj->value - glg->vadj->page_size;

   /* Draw the x tick layout */
   for (i = glg->left, j = 0; j <= steps; i = i + inc, j++) {
      pixbuf = g_list_nth_data(glg->axes[GTKGLG_AXIS_X].major_tick_pixbuf_list, j);
      if (pixbuf == NULL) {
         continue;
      }
      width = gdk_pixbuf_get_width(pixbuf);
      height = gdk_pixbuf_get_height(pixbuf);

      x = i - ((gdouble) width) / 2.0;

      if (x < xc || y < yc) {
         /* Raster pos. would be invalid */
         glRasterPos2d(xc + 0.01, yc + 0.01);   /* 0.01 prevents rounding error */
         glBitmap(0, 0, 0, 0, x - xc, y - yc, NULL);
      } else {
         glRasterPos2d(x, y);
      }

      /* Draw the x axis title */
      pixels = gdk_pixbuf_get_pixels(pixbuf);
      glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid *) pixels);
   }
}

static void gtk_glgraph_draw_y_axis(const GtkGLGraph * glg)
{
   GtkGLGDrawAxis drawn;

   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   drawn = glg->axes[GTKGLG_AXIS_Y].drawn;

   if (!(drawn & GTKGLG_DA_AXIS)) {
      return;
   }

   /* Set Colour to Black */
   glColor3f(0.0, 0.0, 0.0);
   glLineWidth(1.0);

   /* Draw what needs to be drawn */
   if (drawn & GTKGLG_DA_INSIDE_TICK_MARKS) {
      gtk_glgraph_draw_y_inside_tick_marks(glg);
   }
   if (drawn & GTKGLG_DA_OUTSIDE_TICK_MARKS) {
      gtk_glgraph_draw_y_outside_tick_marks(glg);
   }
   if (drawn & GTKGLG_DA_MAJOR_GRID) {
      gtk_glgraph_draw_y_major_grid(glg);
   }
   if (drawn & GTKGLG_DA_MINOR_GRID) {
      gtk_glgraph_draw_y_minor_grid(glg);
   }
   if (drawn & GTKGLG_DA_TITLE) {
      gtk_glgraph_draw_y_title(glg);
   }
   if (drawn & GTKGLG_DA_MAJOR_TICK_TEXT) {
      gtk_glgraph_draw_y_major_tick_text(glg);
   }
}

static void gtk_glgraph_draw_y_inside_tick_marks(const GtkGLGraph * glg)
{
   gdouble inc;
   gdouble xl, xr, i;
   gint j, steps;

   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   /* Figure out whats known */
   xl = glg->left;
   xr = glg->w - glg->right;

   steps = glg->axes[GTKGLG_AXIS_Y].major_steps;
   inc = (glg->h - glg->top - glg->bottom) / (gdouble) steps;

   /* Draw the vertical inside ticks */
   glBegin(GL_LINES);
   for (i = glg->bottom, j = 0; j <= steps; i = i + inc, j++) {
      glVertex2d(xl, i);
      glVertex2d(xl + 5.0, i);
      glVertex2d(xr, i);
      glVertex2d(xr - 5.0, i);
   }
   glEnd();
}

static void gtk_glgraph_draw_y_outside_tick_marks(const GtkGLGraph * glg)
{
   gdouble inc;
   gdouble xl, xr, i;
   gint j, steps;

   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   /* Figure out whats known */
   xl = glg->left;
   xr = glg->w - glg->right;

   steps = glg->axes[GTKGLG_AXIS_Y].major_steps;
   inc = (glg->h - glg->top - glg->bottom) / (gdouble) steps;

   /* Draw the vertical outside ticks */
   glBegin(GL_LINES);
   for (i = glg->bottom, j = 0; j <= steps; i = i + inc, j++) {
      glVertex2d(xl, i);
      glVertex2d(xl - 5.0, i);
      glVertex2d(xr, i);
      glVertex2d(xr + 5.0, i);
   }
   glEnd();
}

static void gtk_glgraph_draw_y_major_grid(const GtkGLGraph * glg)
{
   gdouble inc;
   gdouble xl, xr, i;
   gint j, steps;

   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   /* Figure out whats known */
   xl = glg->left;
   xr = glg->w - glg->right;

   steps = glg->axes[GTKGLG_AXIS_Y].major_steps;
   inc = (glg->h - glg->top - glg->bottom) / (gdouble) steps;

   /* Draw the vertical major grid */
   glBegin(GL_LINES);
   for (i = glg->bottom, j = 0; j <= steps; i = i + inc, j++) {
      glVertex2d(xl, i);
      glVertex2d(xr, i);
   }
   glEnd();
}

static void gtk_glgraph_draw_y_minor_grid(const GtkGLGraph * glg)
{
   gdouble inc;
   gdouble xl, xr, i, k;
   gint j, major_steps, minor_steps;

   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   /* Set Colour to Gray */
   glColor3f(0.7, 0.7, 0.7);

   /* Figure out whats known */
   xl = glg->left;
   xr = glg->w - glg->right;

   minor_steps = glg->axes[GTKGLG_AXIS_Y].minor_steps;
   major_steps = glg->axes[GTKGLG_AXIS_Y].major_steps;
   inc = (glg->h - glg->top -
      glg->bottom) / (gdouble) major_steps / (gdouble) minor_steps;

   /* Draw the vertical minor grid */
   glBegin(GL_LINES);
   for (i = glg->bottom + inc, j = 0; j < major_steps; i = i + inc, j++) {
      for (k = 1; k < minor_steps; i = i + inc, k++) {
         glVertex2d(xl, i);
         glVertex2d(xr, i);
      }
   }
   glEnd();
}

static void gtk_glgraph_draw_y_title(const GtkGLGraph * glg)
{
   gdouble y, x;
   gint width, height;
   const guchar *pixels;
   gdouble xc, yc;

   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   if (glg->axes[GTKGLG_AXIS_Y].pixbuf == NULL) {
      return;
   }

   /* Figure out whats known */
   height = gdk_pixbuf_get_height(glg->axes[GTKGLG_AXIS_Y].pixbuf);
   width = gdk_pixbuf_get_width(glg->axes[GTKGLG_AXIS_Y].pixbuf);

   /* Find the left point */
   x = GTKGLG_TEXT;
   y = (glg->h - glg->top + glg->bottom - (gdouble) height) / 2.0;

   /* Get the Clipping Area */
   /* Stupid hack because openGL requires RasterPos to be in the viewport */
   xc = glg->hadj->value;
   yc = glg->vadj->upper - glg->vadj->value - glg->vadj->page_size;

   if (x < xc || y < yc) {
      /* Raster pos. would be invalid */
      glRasterPos2d(xc + 0.01, yc + 0.01);      /* 0.01 prevents rounding error */
      glBitmap(0, 0, 0, 0, x - xc, y - yc, NULL);
   } else {
      glRasterPos2d(x, y);
   }

   /* Draw the y axis title */
   pixels = gdk_pixbuf_get_pixels(glg->axes[GTKGLG_AXIS_Y].pixbuf);
   glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid *) pixels);
}

static void gtk_glgraph_draw_y_major_tick_text(const GtkGLGraph * glg)
{
   gint32 j, steps;
   GdkPixbuf *pixbuf;
   gint width, height;
   gdouble inc, i;
   gdouble x, y;
   const guchar *pixels;
   gdouble xc, yc;

   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   if (glg->axes[GTKGLG_AXIS_Y].major_tick_pixbuf_list == NULL) {
      return;
   }

   steps = glg->axes[GTKGLG_AXIS_Y].major_steps;
   inc = (glg->h - glg->top - glg->bottom) / (gdouble) steps;

   /* Get the Clipping Area */
   /* Stupid hack because openGL requires RasterPos to be in the viewport */
   xc = glg->hadj->value;
   yc = glg->vadj->upper - glg->vadj->value - glg->vadj->page_size;

   /* Draw the y tick layout */
   for (i = glg->bottom, j = 0; j <= steps; i = i + inc, j++) {
      pixbuf = g_list_nth_data(glg->axes[GTKGLG_AXIS_Y].major_tick_pixbuf_list, j);
      if (pixbuf == NULL) {
         continue;
      }
      width = gdk_pixbuf_get_width(pixbuf);
      height = gdk_pixbuf_get_height(pixbuf);

      x = glg->left - GTKGLG_TEXT - (gdouble) width;
      y = i - ((gdouble) height) / 2.0;

      if (x < xc || y < yc) {
         /* Raster pos. would be invalid */
         glRasterPos2d(xc + 0.01, yc + 0.01);   /* 0.01 prevents rounding error */
         glBitmap(0, 0, 0, 0, x - xc, y - yc, NULL);
      } else {
         glRasterPos2d(x, y);
      }

      /* Draw the y axis title */
      pixels = gdk_pixbuf_get_pixels(pixbuf);
      glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid *) pixels);
   }
}

static void gtk_glgraph_draw_z_axis(const GtkGLGraph * glg)
{
   GtkGLGDrawAxis drawn;

   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   drawn = glg->axes[GTKGLG_AXIS_Z].drawn;

   if (!(drawn & GTKGLG_DA_AXIS)) {
      return;
   }

   /* Set Colour to Black */
   glColor3f(0.0, 0.0, 0.0);
   glLineWidth(1.0);

   /* Draw what needs to be drawn */
   if (drawn & GTKGLG_DA_INSIDE_TICK_MARKS) {
      gtk_glgraph_draw_z_inside_tick_marks(glg);
   }
   if (drawn & GTKGLG_DA_OUTSIDE_TICK_MARKS) {
      gtk_glgraph_draw_z_outside_tick_marks(glg);
   }
   if (drawn & GTKGLG_DA_MAJOR_GRID) {
      gtk_glgraph_draw_z_major_grid(glg);
   }
   if (drawn & GTKGLG_DA_MINOR_GRID) {
      gtk_glgraph_draw_z_minor_grid(glg);
   }
   if (drawn & GTKGLG_DA_TITLE) {
      gtk_glgraph_draw_z_title(glg);
   }
   if (drawn & GTKGLG_DA_MAJOR_TICK_TEXT) {
      gtk_glgraph_draw_z_major_tick_text(glg);
   }

   gtk_glgraph_draw_z_scale(glg);
   gtk_glgraph_draw_z_graph_box(glg);
}

static void gtk_glgraph_draw_z_inside_tick_marks(const GtkGLGraph * glg)
{
   gdouble inc;
   gdouble zl, zr, i;
   gint j, steps;

   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   /* Figure out whats known */
   zl = glg->w - glg->z_left;
   zr = glg->w - glg->z_right;

   steps = glg->axes[GTKGLG_AXIS_Z].major_steps;
   inc = (glg->h - glg->top - glg->bottom) / (gdouble) steps;

   /* Draw the x inside ticks */
   glBegin(GL_LINES);
   for (i = glg->bottom, j = 0; j <= steps; i = i + inc, j++) {
      glVertex2d(zl, i);
      glVertex2d(zl + 5.0, i);
      glVertex2d(zr, i);
      glVertex2d(zr - 5.0, i);
   }
   glEnd();
}

static void gtk_glgraph_draw_z_outside_tick_marks(const GtkGLGraph * glg)
{
   gdouble inc;
   gdouble zl, zr, i;
   gint j, steps;

   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   /* Figure out whats known */
   zl = glg->w - glg->z_left;
   zr = glg->w - glg->z_right;

   steps = glg->axes[GTKGLG_AXIS_Z].major_steps;
   inc = (glg->h - glg->top - glg->bottom) / (gdouble) steps;

   /* Draw the x outside ticks */
   glBegin(GL_LINES);
   for (i = glg->bottom, j = 0; j <= steps; i = i + inc, j++) {
      glVertex2d(zl, i);
      glVertex2d(zl - 5.0, i);
      glVertex2d(zr, i);
      glVertex2d(zr + 5.0, i);
   }
   glEnd();
}

static void gtk_glgraph_draw_z_major_grid(const GtkGLGraph * glg)
{
   gdouble inc;
   gdouble zl, zr, i;
   gint j, steps;

   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   /* Figure out whats known */
   zl = glg->w - glg->z_left;
   zr = glg->w - glg->z_right;

   steps = glg->axes[GTKGLG_AXIS_Z].major_steps;
   inc = (glg->h - glg->top - glg->bottom) / (gdouble) steps;

   /* Draw the z major grid */
   glBegin(GL_LINES);
   for (i = glg->bottom, j = 0; j <= steps; i = i + inc, j++) {
      glVertex2d(zl, i);
      glVertex2d(zr, i);
   }
   glEnd();
}

static void gtk_glgraph_draw_z_minor_grid(const GtkGLGraph * glg)
{
   gdouble inc;
   gdouble zl, zr, i, k;
   gint j, major_steps, minor_steps;

   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   /* Set Colour to Gray */
   glColor3f(0.7, 0.7, 0.7);

   /* Figure out whats known */
   zl = glg->w - glg->z_left;
   zr = glg->w - glg->z_right;

   minor_steps = glg->axes[GTKGLG_AXIS_Z].minor_steps;
   major_steps = glg->axes[GTKGLG_AXIS_Z].major_steps;
   inc = (glg->h - glg->top -
      glg->bottom) / (gdouble) major_steps / (gdouble) minor_steps;

   /* Draw the z minor grid */
   glBegin(GL_LINES);
   for (i = glg->bottom + inc, j = 0; j < major_steps; i = i + inc, j++) {
      for (k = 1; k < minor_steps; i = i + inc, k++) {
         glVertex2d(zl, i);
         glVertex2d(zr, i);
      }
   }
   glEnd();
}

static void gtk_glgraph_draw_z_scale(const GtkGLGraph * glg)
{
   gdouble steps, inc, j;
   gdouble zl, zr, i;
   GtkGLGraphColormap cmap;
   gdouble alpha;

   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   cmap = GTKGLG_COLORMAP_JET;
   alpha = 0.9;

   if (glg->datasets != NULL) {
      GtkGLGDataSet *glds;

      glds = g_list_nth_data(glg->datasets, 0);
      if (glds != NULL) {
         cmap = glds->glgcmap;
         alpha = glds->glgcmap_alpha;
      }
   }

   glEnable(GL_BLEND);
   glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

   /* Figure out whats known */
   zl = glg->w - glg->z_left;
   zr = glg->w - glg->z_right;
   steps = 512.0;

   inc = (glg->h - glg->top - glg->bottom) / steps;
   /* Draw the horizontal minor grid */
   glBegin(GL_QUAD_STRIP);
   for (i = 0.0, j = glg->bottom; i <= steps; i++, j += inc) {
      gtk_glgraph_set_color(glg, (i / steps), alpha, cmap, 1, 0);
      glVertex2d(zl, j);
      glVertex2d(zr, j);
   }
   glEnd();
}

static void gtk_glgraph_draw_z_graph_box(const GtkGLGraph * glg)
{

   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   /* Set Colour to Black */
   glColor3f(0.0, 0.0, 0.0);
   glLineWidth(2.0);

   /* Draw the Axes */
   glBegin(GL_LINE_LOOP);
   glVertex2d(glg->w - glg->z_left, glg->bottom);
   glVertex2d(glg->w - glg->z_right, glg->bottom);
   glVertex2d(glg->w - glg->z_right, glg->h - glg->top);
   glVertex2d(glg->w - glg->z_left, glg->h - glg->top);
   glEnd();
}

static void gtk_glgraph_draw_z_title(const GtkGLGraph * glg)
{
   gdouble y, x;
   gint width, height;
   const guchar *pixels;
   gdouble xc, yc;

   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   if (glg->axes[GTKGLG_AXIS_Z].pixbuf == NULL) {
      return;
   }

   /* Figure out whats known */
   height = gdk_pixbuf_get_height(glg->axes[GTKGLG_AXIS_Z].pixbuf);
   width = gdk_pixbuf_get_width(glg->axes[GTKGLG_AXIS_Z].pixbuf);

   /* Find the left point */
   x = glg->w - GTKGLG_TEXT - (gdouble) width;
   y = (glg->h - glg->top + glg->bottom - (gdouble) height) / 2.0;

   /* Get the Clipping Area */
   /* Stupid hack because openGL requires RasterPos to be in the viewport */
   xc = glg->hadj->value;
   yc = glg->vadj->upper - glg->vadj->value - glg->vadj->page_size;

   if (x < xc || y < yc) {
      /* Raster pos. would be invalid */
      glRasterPos2d(xc + 0.01, yc + 0.01);      /* 0.01 prevents rounding error */
      glBitmap(0, 0, 0, 0, x - xc, y - yc, NULL);
   } else {
      glRasterPos2d(x, y);
   }

   /* Draw the z axis title */
   pixels = gdk_pixbuf_get_pixels(glg->axes[GTKGLG_AXIS_Z].pixbuf);
   glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid *) pixels);
}

static void gtk_glgraph_draw_z_major_tick_text(const GtkGLGraph * glg)
{
   gint32 j, steps;
   GdkPixbuf *pixbuf;
   gint width, height;
   gdouble inc, i;
   gdouble x, y;
   const guchar *pixels;
   gdouble xc, yc;

   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   if (glg->axes[GTKGLG_AXIS_Z].major_tick_pixbuf_list == NULL) {
      return;
   }

   steps = glg->axes[GTKGLG_AXIS_Z].major_steps;
   inc = (glg->h - glg->top - glg->bottom) / (gdouble) steps;

   /* Get the Clipping Area */
   /* Stupid hack because openGL requires RasterPos to be in the viewport */
   xc = glg->hadj->value;
   yc = glg->vadj->upper - glg->vadj->value - glg->vadj->page_size;

   /* Draw the y tick layout */
   for (i = glg->bottom, j = 0; j <= steps; i = i + inc, j++) {
      pixbuf = g_list_nth_data(glg->axes[GTKGLG_AXIS_Z].major_tick_pixbuf_list, j);
      if (pixbuf == NULL) {
         continue;
      }
      width = gdk_pixbuf_get_width(pixbuf);
      height = gdk_pixbuf_get_height(pixbuf);

      x = glg->w - glg->z_right + GTKGLG_TEXT;
      y = i - ((gdouble) height) / 2.0;

      if (x < xc || y < yc) {
         /* Raster pos. would be invalid */
         glRasterPos2d(xc + 0.01, yc + 0.01);   /* 0.01 prevents rounding error */
         glBitmap(0, 0, 0, 0, x - xc, y - yc, NULL);
      } else {
         glRasterPos2d(x, y);
      }

      /* Draw the y axis title */
      pixels = gdk_pixbuf_get_pixels(pixbuf);
      glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid *) pixels);
   }
}

static void gtk_glgraph_layout_calculate_offsets(GtkGLGraph * glg)
{
   gint width, height;
   GtkGLGDrawAxis drawn;
   GList *datasets;
   GtkGLGDataSet *dataset;

   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   glg->top = BORDER;
   glg->bottom = BORDER;
   glg->left = BORDER;
   glg->right = BORDER;
   glg->z_left = 0.0;
   glg->z_right = 0.0;
   glg->l_left = 0.0;

   /* Title */
   drawn = glg->drawn;
   if (drawn & GTKGLG_D_TITLE) {
      if (glg->title_pixbuf != NULL) {
         height = gdk_pixbuf_get_height(glg->title_pixbuf);
         glg->top += height + GTKGLG_TEXT;
      }
   }

   /* X Axis */
   drawn = glg->axes[GTKGLG_AXIS_X].drawn;
   if (drawn & GTKGLG_DA_AXIS) {
      if (drawn & GTKGLG_DA_MAJOR_TICK_TEXT) {
         glg->bottom += TEXT + (gdouble) glg->axes[GTKGLG_AXIS_X].tick_height;
         /* If no z axis, give room for rest of x axis ticks */
         if (!(glg->axes[GTKGLG_AXIS_Z].drawn & GTKGLG_DA_AXIS)) {
            glg->right += (((gdouble) glg->axes[GTKGLG_AXIS_X].tick_width) * 0.5);
         }
      }

      if (drawn & GTKGLG_DA_TITLE) {
         if (glg->axes[GTKGLG_AXIS_X].title_layout) {
            pango_layout_get_pixel_size(glg->axes[GTKGLG_AXIS_X].title_layout,
               &width, &height);
            glg->bottom += TEXT + (gdouble) height;
         }
      }
   }

   /* Y Axis */
   drawn = glg->axes[GTKGLG_AXIS_Y].drawn;
   if (drawn & GTKGLG_DA_AXIS) {
      if (drawn & GTKGLG_DA_MAJOR_TICK_TEXT) {
         glg->left += TEXT + (gdouble) glg->axes[GTKGLG_AXIS_Y].tick_width;
      }

      if (drawn & GTKGLG_DA_TITLE) {
         if (glg->axes[GTKGLG_AXIS_Y].title_layout) {
            pango_layout_get_pixel_size(glg->axes[GTKGLG_AXIS_Y].title_layout,
               &width, &height);
            glg->left += TEXT + (gdouble) height;
         }
      }
   }

   /* Z Axis */
   drawn = glg->axes[GTKGLG_AXIS_Z].drawn;
   if (drawn & GTKGLG_DA_AXIS) {
      glg->right += BORDER + Z_WIDTH;
      glg->z_left += BORDER + Z_WIDTH;
      glg->z_right += BORDER;
      if (drawn & GTKGLG_DA_MAJOR_TICK_TEXT) {
         glg->right += TEXT + (gdouble) glg->axes[GTKGLG_AXIS_Z].tick_width;
         glg->z_left += TEXT + (gdouble) glg->axes[GTKGLG_AXIS_Z].tick_width;
         glg->z_right += TEXT + (gdouble) glg->axes[GTKGLG_AXIS_Z].tick_width;
      }
      if (drawn & GTKGLG_DA_TITLE) {
         if (glg->axes[GTKGLG_AXIS_Z].title_layout) {
            pango_layout_get_pixel_size(glg->axes[GTKGLG_AXIS_Z].title_layout,
               &width, &height);
            glg->right += TEXT + (gdouble) height;
            glg->z_left += TEXT + (gdouble) height;
            glg->z_right += TEXT + (gdouble) height;
         }
      }
   }

   /* Legend */
   drawn = glg->drawn;
   if (drawn & GTKGLG_D_LEGEND) {
      /* Parse all the current GLDS and find max title width */
      glg->legend_text_width = 0.0;

      datasets = g_list_first(glg->datasets);
      while (datasets) {
         dataset = datasets->data;
         datasets = datasets->next;
         if (dataset == NULL) {
            continue;
         }
         if (dataset->title_pixbuf != NULL) {
            glg->legend_text_width =
               MAX(glg->legend_text_width,
               (gdouble) gdk_pixbuf_get_width(dataset->title_pixbuf));
         }
      }

      if (glg->legend_in_out == FALSE) {
         glg->right +=
            BORDER + glg->legend_text_width + 3 * LEGEND_MARGIN + LEGEND_LINE_LENGTH;
         glg->z_left +=
            BORDER + glg->legend_text_width + 3 * LEGEND_MARGIN + LEGEND_LINE_LENGTH;
         glg->z_right +=
            BORDER + glg->legend_text_width + 3 * LEGEND_MARGIN + LEGEND_LINE_LENGTH;
         glg->l_left +=
            BORDER + glg->legend_text_width + 3 * LEGEND_MARGIN + LEGEND_LINE_LENGTH;
      }
   }
}

static void gtk_glgraph_update_axis_ticks(GtkGLGraph * const glg,
   GtkGLGraphAxis * const axis)
{
   PangoLayout *layout = NULL;
   GList *temp_list = NULL;
   gdouble val, inc;
   gint32 i;
   gchar *buf;
   GtkWidget *widget = NULL;
   GList *pixbuf_list = NULL;
   GdkPixbuf *pixbuf = NULL;
   gint width, height;

   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   /* we can't handle that case */
   if (axis == NULL) {
      return;
   }

   /* Free the old pixbuf list */
   pixbuf_list = axis->major_tick_pixbuf_list;

   /* Free the old list */
   if (pixbuf_list != NULL) {
      temp_list = g_list_first(pixbuf_list);
      while (temp_list) {
         pixbuf = temp_list->data;
         temp_list = temp_list->next;
         g_object_unref(G_OBJECT(pixbuf));
      }
      g_list_free(pixbuf_list);
      pixbuf_list = NULL;
   }

   /* Create the new pixbuf list */
   widget = GTK_WIDGET(glg);
   layout = gtk_widget_create_pango_layout(widget, "");
   val = axis->min;
   inc = (axis->max - axis->min) / (gdouble) axis->major_steps;
   axis->tick_width = 0;
   axis->tick_height = 0;
   for (i = 0; i <= axis->major_steps; i++, val += inc) {
      buf = g_strdup_printf("%01.*f", axis->precision, val);
      pango_layout_set_text(layout, buf, -1);
      g_free(buf);

      /* Record max layout size */
      pango_layout_get_pixel_size(layout, &width, &height);
      axis->tick_width = MAX(width, axis->tick_width);
      axis->tick_height = MAX(height, axis->tick_height);

      /* Generate the pixbuf */
      pixbuf = gtk_glgraph_create_rotated_flipped_text_pixbuf(glg, layout,
         GTKGLG_DIRECTION_0, FALSE, TRUE);
      pixbuf_list = g_list_prepend(pixbuf_list, pixbuf);
   }
   g_object_unref(G_OBJECT(layout));
   pixbuf_list = g_list_reverse(pixbuf_list);

   axis->major_tick_pixbuf_list = pixbuf_list;
}

static GdkPixbuf *gtk_glgraph_create_rotated_flipped_text_pixbuf(GtkGLGraph * glg,
   PangoLayout *
   layout, GtkGLGraphDirection dir, gboolean horizontal, gboolean vertical)
{
   GdkPixmap *norm_pixmap = NULL;
   gint width, height;
   gint rot_width, rot_height;
   GtkWidget *widget;
   GdkPixbuf *norm_pixbuf = NULL, *rot_pixbuf = NULL;
   guint32 *norm_pix, *rot_pix;
   gint i, j, k, l;

   g_return_val_if_fail(glg != NULL, NULL);
   g_return_val_if_fail(GTK_IS_GLGRAPH(glg), NULL);
   g_return_val_if_fail(layout != NULL, NULL);

   widget = GTK_WIDGET(glg);

   /* FIGURE OUT THE SIZES */

   /* Get Layout Size */
   pango_layout_get_pixel_size(layout, &width, &height);
   /* Silently fail if the layout is too small */
   if (width <= 0 || height <= 0) {
      return NULL;
   }

   /* Figure out the rotated width and height */
   if (dir == GTKGLG_DIRECTION_270 || dir == GTKGLG_DIRECTION_90) {
      rot_width = height;
      rot_height = width;
   } else {
      rot_height = height;
      rot_width = width;
   }
   /* Allocate Server side buffer */
   norm_pixmap = gdk_pixmap_new(widget->window, width, height, -1);

   /* DRAW THE LAYOUT INTO THE PIXMAP */

   /* Paint white so that we see something */
   gdk_draw_rectangle(norm_pixmap, widget->style->white_gc, TRUE, 0, 0, width,
      height);
   /* Draw the layout */
   gdk_draw_layout(norm_pixmap, widget->style->text_gc[0], 0, 0, layout);

   /* ALLOCATE A BUFFER FOR TEH NORMAL PIXBUF */

   /* Allocate Clientside buffer */
   norm_pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, width, height);
   /* Transfer buffer from server to client */
   norm_pixbuf =
      gdk_pixbuf_get_from_drawable(norm_pixbuf, norm_pixmap, NULL, 0, 0, 0, 0,
      width, height);
   /* Get the raw pixel pointer of client buffer */
   norm_pix = (guint32 *) gdk_pixbuf_get_pixels(norm_pixbuf);

   /* FLIP HORINZONTALLY */

   /* Flip the pixbuf Horizontally if needed */
   if (horizontal == TRUE) {
      gint rows, cols;
      guint32 *row;

      rows = gdk_pixbuf_get_height(norm_pixbuf);
      cols = gdk_pixbuf_get_width(norm_pixbuf);
      row = g_malloc(cols * sizeof(guint32));

      for (j = 0; j < rows; j++) {
         /* Record the data */
         for (k = 0; k < cols; k++) {
            row[k] = norm_pix[j * cols + k];
         }
         /* Put it back in reverse order */
         for (k = 0, i = (cols - 1); k < cols; k++, i--) {
            norm_pix[j * cols + k] = row[i];
         }
      }
      g_free(row);
   }

   /* FLIP VERTICALLY */

   /* Flip the pixbuf Horizontally if needed */
   if (vertical == TRUE) {
      gint rows, cols;
      guint32 *col;

      rows = gdk_pixbuf_get_height(norm_pixbuf);
      cols = gdk_pixbuf_get_width(norm_pixbuf);
      col = g_malloc(rows * sizeof(guint32));

      for (j = 0; j < cols; j++) {
         /* Record the data */
         for (k = 0; k < rows; k++) {
            col[k] = norm_pix[k * cols + j];
         }
         /* Put it back in reverse order */
         for (k = 0, i = (rows - 1); k < rows; k++, i--) {
            norm_pix[k * cols + j] = col[i];
         }
      }
      g_free(col);
   }

   /* Allocate a new client buffer with rotated memory */
   rot_pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, rot_width, rot_height);
   /* Get its pixels */
   rot_pix = (guint32 *) gdk_pixbuf_get_pixels(rot_pixbuf);

   /* Actually rotate */
   switch (dir) {
   case GTKGLG_DIRECTION_90:
      k = 0;
      for (j = width - 1; j >= 0; j--) {
         l = j;
         for (i = 0; i < height; i++, k++, l += width) {
            rot_pix[k] = norm_pix[l];
         }
      }
      break;
   case GTKGLG_DIRECTION_180:
      k = width * height - 1;
      for (j = k, i = 0; i <= k; i++, j--) {
         rot_pix[i] = norm_pix[j];
      }
      break;
   case GTKGLG_DIRECTION_270:
      k = 0;
      for (j = 0; j < width; j++) {
         l = j + (height - 1) * width;
         for (i = height - 1; i >= 0; i--, k++, l -= width) {
            rot_pix[k] = norm_pix[l];
         }
      }
      break;
   default:
      /* Just copy the pixels */
      memcpy(rot_pix, norm_pix, (width * height * 4));
   }

   /* Free everything */
   g_object_unref(G_OBJECT(norm_pixmap));
   g_object_unref(G_OBJECT(norm_pixbuf));

   /* return */
   return rot_pixbuf;
}

static void gtk_glgraph_pixbuf_update_tooltip(GtkGLGraph * const glg,
   gchar * const text)
{
   GtkWidget *widget;
   PangoLayout *layout;

   g_return_if_fail(glg != NULL);
   g_return_if_fail(GTK_IS_GLGRAPH(glg));

   if (glg->tooltip_pixbuf) {
      g_object_unref(G_OBJECT(glg->tooltip_pixbuf));
   }

   widget = GTK_WIDGET(glg);
   layout = gtk_widget_create_pango_layout(widget, NULL);
   pango_layout_set_markup(layout, text, -1);
   glg->tooltip_pixbuf =
      gtk_glgraph_create_rotated_flipped_text_pixbuf(glg, layout,
      GTKGLG_DIRECTION_0, FALSE, TRUE);
   g_object_unref(G_OBJECT(layout));
}

static void g_cclosure_user_marshal_VOID__OBJECT_OBJECT(GClosure * closure,
   GValue * return_value,
   guint n_param_values,
   const GValue * param_values, gpointer invocation_hint, gpointer marshal_data)
{
   typedef void (*GMarshalFunc_VOID__OBJECT_OBJECT) (gpointer data1,
      gpointer arg_1, gpointer arg_2, gpointer data2);
   register GMarshalFunc_VOID__OBJECT_OBJECT callback;
   register GCClosure *cc = (GCClosure *) closure;
   register gpointer data1, data2;

   g_return_if_fail(n_param_values == 3);

   if (G_CCLOSURE_SWAP_DATA(closure)) {
      data1 = closure->data;
      data2 = g_value_peek_pointer(param_values + 0);
   } else {
      data1 = g_value_peek_pointer(param_values + 0);
      data2 = closure->data;
   }
   callback =
      (GMarshalFunc_VOID__OBJECT_OBJECT) (marshal_data ? marshal_data : cc->
      callback);

   callback(data1, g_marshal_value_peek_object(param_values + 1),
      g_marshal_value_peek_object(param_values + 2), data2);
}
