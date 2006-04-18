/* gapcmon_gtkglgraph.h    serial-0070-0 ***************************************
 *
 *  original package called "GtkGLGraph" by Todd Goyen
 *      Thu Jun 10 22:50:18 2004
 *      Copyright  2004  Todd Goyen (GPL)
 *      tgoyen@swri.org or me@mumblelina.com
 *      http://mumblelina.com
 *
 * modified by James Scott, Jr.<skoona@users.sourceforge.net>
 *  March, 2006 (GPL)
 *    -  repacking for use in gapcmon.sourceforge.net
 *    -  enabled legend support and partial tooltip
 *    -  created a example program for the library showing linegraph
 *    -  added gtk_glgraph_unrealize()  to fix Xerrors
 *    -  added gtk_glgraph_destroy() for normal cleanup
 ***************************************************************************
*/
#ifndef __GTK_GLGRAPH_H__
#define __GTK_GLGRAPH_H__

#ifdef G_OS_UNIX
#include <gdk/gdk.h>
#include <GL/glx.h>
#include <gdk/gdkx.h>
#endif                             /* linux */

#ifdef G_OS_WIN32
#include <gdk/gdkwin32.h>
#endif                             /* WINDOWS */

#include <GL/glu.h>
#include <GL/gl.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define GTK_TYPE_GLGRAPH			(gtk_glgraph_get_type ())
#define GTK_GLGRAPH(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_GLGRAPH, GtkGLGraph))
#define GTK_GLGRAPH_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_GLGRAPH, GtkGLGraphClass))
#define GTK_IS_GLGRAPH(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_GLGRAPH))
#define GTK_IS_GLGRAPH_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_GLGRAPH))
#define GTK_GLGRAPH_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_GLGRAPH, GtkGLGraphClass))
#define GTKGLG_TEXT		6.0
#define BORDER	15.0
#define LEGEND_MARGIN	3.0
#define LEGEND_LINE_LENGTH	15.0
   typedef struct _GtkGLGraphAxis GtkGLGraphAxis;
   typedef struct _GtkGLGDataSet GtkGLGDataSet;
   typedef struct _GtkGLGraph GtkGLGraph;
   typedef struct _GtkGLGraphClass GtkGLGraphClass;

   typedef enum {
      GTKGLG_DIRECTION_0,
      GTKGLG_DIRECTION_90,
      GTKGLG_DIRECTION_180,
      GTKGLG_DIRECTION_270,
      GTKGLG_DIRECTION_LAST
   } GtkGLGraphDirection;

   typedef enum {
      GTKGLG_AXIS_MODE_EQUAL,      /* graph ensure that minor step size in both axes is equal */
      GTKGLG_AXIS_MODE_SQUARE,     /* Not implemented */
      GTKGLG_AXIS_MODE_FILL,       /* Zooming is disabled and the graph is resized to fill allocated space */
      GTKGLG_AXIS_MODE_ASPECTRATIO,     /* User defined aspect ratio */
      GTKGLG_AXIS_MODE_LAST
   } GtkGLGraphAxisMode;

   typedef enum {
      GTKGLG_AXIS_X,
      GTKGLG_AXIS_Y,
      GTKGLG_AXIS_Z,
      GTKGLG_AXIS_LAST
   } GtkGLGraphAxisType;

   typedef enum {
      GTKGLG_TYPE_XY,

/*	GTKGLG_TYPE_XY_AKIMA_SPLINE, */

/*	GTKGLG_TYPE_XY_B_SPLINE, */

/*	GTKGLG_TYPE_XY_BEZIER_SPLINE, */

/*	GTKGLG_TYPE_XY_CUBIC_SPLINE, */

/*	GTKGLG_TYPE_VERTICAL_BAR, */

/*	GTKGLG_TYPE_HORIZONTAL_BAR, */
      GTKGLG_TYPE_SURFACE,
      GTKGLG_TYPE_SURFACE_BICUBIC_INTERPOLATION,

/*	GTKGLG_TYPE_SURFACE_SPLINE, */
      GTKGLG_TYPE_LAST
   } GtkGLGraphType;

   typedef enum {
      GTKGLG_COLORMAP_GRAYSCALE,
      GTKGLG_COLORMAP_REDSCALE,
      GTKGLG_COLORMAP_GREENSCALE,
      GTKGLG_COLORMAP_BLUESCALE,
      GTKGLG_COLORMAP_HSV,
      GTKGLG_COLORMAP_JET,
      GTKGLG_COLORMAP_LAST
   } GtkGLGraphColormap;

   typedef enum {
      GTKGLG_DA_AXIS = 0x01,
      GTKGLG_DA_INSIDE_TICK_MARKS = 0x02,
      GTKGLG_DA_OUTSIDE_TICK_MARKS = 0x04,
      GTKGLG_DA_MAJOR_GRID = 0x08,
      GTKGLG_DA_MINOR_GRID = 0x10,
      GTKGLG_DA_TITLE = 0x20,
      GTKGLG_DA_MAJOR_TICK_TEXT = 0x40
   } GtkGLGDrawAxis;

   typedef enum {
      GTKGLG_D_TITLE = 0x01,
      GTKGLG_D_CURSOR_BOX = 0x02,
      GTKGLG_D_HORIZONTAL_CURSOR = 0x04,
      GTKGLG_D_VERTICAL_CURSOR = 0x08,
      GTKGLG_D_TOOLTIP = 0x10,
      GTKGLG_D_LEGEND = 0x20
   } GtkGLGDraw;

   struct _GtkGLGraphAxis {
      gdouble min;
      gdouble max;
      gint32 major_steps;          /* >= 1 */
      gint32 minor_steps;          /* >= 1 */
      gint8 precision;

      gchar *title;
      PangoLayout *title_layout;
      GdkPixbuf *pixbuf;
      GList *major_tick_pixbuf_list;
      gint tick_height;
      gint tick_width;

      GtkGLGDrawAxis drawn;
   };

   struct _GtkGLGDataSet {
      GtkGLGraphType graph_type;

      gdouble *x;   /* data values */
      gdouble *y;
      gdouble *z;
      gint x_length;
      gint y_length;
      gint z_length;
      gdouble *xi;
      gdouble *yi;
      gdouble *zi;
      gdouble xi_length;
      gdouble yi_length;

      gchar *x_units;
      gchar *y_units;
      gchar *z_units;

      gdouble line_color[4];       /* RGBA 0..1 */
      gchar  *line_color_name;      
      gdouble point_color[4];      /* RGBA 0..1 */
      GtkGLGraphColormap glgcmap;
      gdouble glgcmap_alpha;

      gboolean draw_points;
      gboolean draw_lines;
      gboolean stipple_line;

      gdouble line_width;
      guint16 line_stipple;
      gdouble point_size;

      GdkPixbuf *title_pixbuf;
   };

   struct _GtkGLGraph {
      GtkWidget widget;

      gdouble mx_last;             /* Last Coordinates of Mouse */
      gdouble my_last;             /* Last Coordinates of Mouse */

      /* OpenGL Variables */
#ifdef G_OS_UNIX
      Display *dpy;
      GLXContext cx;
      XVisualInfo *vi;
      GdkColormap *colormap;
#endif                             /* linux */
#ifdef G_OS_WIN32
      HDC hDC;
      HGLRC hRC;
#endif

      GtkGLGDraw drawn;
      gboolean tooltip_visible;
      gboolean tooltip_id_valid;
      guint tooltip_id;
      guint expose_id;
      gdouble ttx;
      gdouble tty;

      /* List of GtkGLDataSet */
      GList *datasets;

      GtkGLGraphAxis axes[GTKGLG_AXIS_LAST];
      GtkGLGraphAxisMode mode;

      /* Axes Border Spacings */
      gdouble top;
      gdouble bottom;
      gdouble left;
      gdouble right;
      gdouble z_left;
      gdouble z_right;
      gdouble l_left;

      gdouble legend_text_width;
      gboolean legend_in_out;      /* JSJ:Added controls placement of legend df:FALSE */

      /* allocated width and height */
      /* updated in expose */
      gdouble w;
      gdouble h;

      gchar *title;
      PangoLayout *title_layout;

      /* Pixbufs */
      GdkPixbuf *title_pixbuf;
      GdkPixbuf *tooltip_pixbuf;

      /* Cursor */
      guint16 cursor_stipple;
      gboolean cursor_moving;
      gdouble cx1;
      gdouble cy1;
      gdouble cx2;
      gdouble cy2;

      /* used by the adjustments */
      GtkAdjustment *hadj;
      GtkAdjustment *vadj;
      gint width;
      gint height;
      gint xoffset;
      gint yoffset;

      gdouble zoom;

      gint queued_draw;
   };

   struct _GtkGLGraphClass {
      GtkWidgetClass parent_class;

      void (*set_scroll_adjustments) (GtkGLGraph * glg,
         GtkAdjustment * hadj, GtkAdjustment * vadj);
      void (*cursor_moved) (GtkGLGraph * glg);
   };

/* Prototypes */
   extern GType gtk_glgraph_get_type(void);
   extern GtkGLGDataSet *gtk_glgraph_dataset_create(void);
   extern GtkGLGDraw gtk_glgraph_get_drawn(GtkGLGraph * glg);
   extern GtkGLGDrawAxis gtk_glgraph_axis_get_visible(GtkGLGraph * glg,
      GtkGLGraphAxisType axis);
   extern GtkGLGraphAxisMode gtk_glgraph_axis_get_mode(const GtkGLGraph * const glg);
   extern GtkWidget *gtk_glgraph_new(void);
   extern const gchar *gtk_glgraph_axis_get_label(const GtkGLGraph * glg,
      GtkGLGraphAxisType axis);
   extern gdouble gtk_glgraph_get_zoom(GtkGLGraph * glg);
   extern void gtk_glgraph_axis_get_range(GtkGLGraph * glg,
      GtkGLGraphAxisType axis,
      gdouble * min, gdouble * max,
      gint32 * major_steps, gint32 * minor_steps, gint8 * precision);
   extern void gtk_glgraph_axis_set_drawn(GtkGLGraph * glg,
      GtkGLGraphAxisType axis, const GtkGLGDrawAxis drawn);
   extern void gtk_glgraph_axis_set_label(GtkGLGraph * glg,
      GtkGLGraphAxisType axis, const gchar * label);
   extern void gtk_glgraph_axis_set_mode(GtkGLGraph * const glg,
      GtkGLGraphAxisMode mode);
   extern void gtk_glgraph_axis_set_range(GtkGLGraph * glg,
      GtkGLGraphAxisType axis,
      gdouble * min, gdouble * max,
      gint32 * major_steps, gint32 * minor_steps, gint8 * precision);
   extern void gtk_glgraph_dataset_add(GtkGLGraph * const glg,
      GtkGLGDataSet * const glds);
   extern void gtk_glgraph_dataset_bicubic_interpolate(GtkGLGDataSet *
      const glds, gint factor);
   extern void gtk_glgraph_dataset_free(GtkGLGDataSet ** glds);
   extern void gtk_glgraph_dataset_free_all(GtkGLGraph * const glg);
   extern void gtk_glgraph_dataset_linear_interpolate(GtkGLGDataSet *
      const glds, gint factor);
   extern void gtk_glgraph_dataset_set_title(GtkGLGraph * const glg,
      GtkGLGDataSet * const glds, const gchar * const title);
   extern void gtk_glgraph_redraw(GtkGLGraph * const glg);
   extern void gtk_glgraph_set_drawn(GtkGLGraph * glg, const GtkGLGDraw drawn);
   extern void gtk_glgraph_set_title(GtkGLGraph * glg, const gchar * const title);
   extern void gtk_glgraph_set_zoom(GtkGLGraph * glg, const gdouble zoom);

G_END_DECLS
#endif                             /* __GTK_GLGRAPH_H__ */
