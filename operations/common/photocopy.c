/* This file is an image processing operation for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 1997 Spencer Kimball
 * Copyright 2011 Hans Lo <hansshulo@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double (mask_radius, _("Mask Radius"), 0.0, 50.0, 10.0,
                   _("Mask Radius"))

gegl_chant_double (sharpness, _("Sharpness"), 0.0, 1.0, 0.5,
                   _("Sharpness"))

gegl_chant_double (black, _("Percent Black"), 0.0, 1.0, 0.2,
                   _("Percent Black"))

gegl_chant_double (white, _("Percent White"), 0.0, 1.0, 0.2,
                   _("Percent White"))

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE       "photocopy.c"

#include "gegl-chant.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define THRESHOLD 0.75

typedef struct {
  gdouble      black;
  gdouble      white;
} Ramps;

static gdouble
compute_ramp (GeglBuffer *input,
	      GeglOperation *operation,
              gdouble  pct,
              gint     under_threshold)
{
  GeglChantO *o                    = GEGL_CHANT_PROPERTIES (operation);

  GeglRectangle *whole_region = gegl_operation_source_get_bounding_box (operation, "input");
  GeglNode *gegl, *image, *write1, *write2, *grey, *blur1, *blur2;
  GeglBuffer *dest1 = gegl_buffer_new(whole_region, babl_format ("Y float"));
  GeglBuffer *dest2 = gegl_buffer_new(whole_region, babl_format ("Y float"));

  gegl = gegl_node_new();
  image = gegl_node_new_child(gegl,
			      "operation", "gegl:buffer-source",
			      "buffer", input,
			      NULL);

  grey = gegl_node_new_child(gegl,
			     "operation", "gegl:grey",
			     NULL);

  blur1 =  gegl_node_new_child(gegl,
			       "operation", "gegl:gaussian-blur",
			       "std_dev_x", (MAX(1.0, 10*(1.0 - o->sharpness)) + 1.0)/sqrt(2.0),
			       "std_dev_y", (MAX(1.0, 10*(1.0 - o->sharpness)) + 1.0)/sqrt(2.0),
			       NULL);
  
  blur2 =  gegl_node_new_child(gegl,
			       "operation", "gegl:gaussian-blur",
			       "std_dev_x", (o->mask_radius + 1.0)/sqrt(2.0),
			       "std_dev_y", (o->mask_radius + 1.0)/sqrt(2.0),
			       NULL);
    
  write1 = gegl_node_new_child(gegl,
			      "operation", "gegl:buffer-sink",
			      "buffer", &dest1, NULL);
  
  write2 = gegl_node_new_child(gegl,
			      "operation", "gegl:buffer-sink",
			      "buffer", &dest2, NULL);
  

  gegl_node_link_many (image, grey, blur1, write1, NULL);
  gegl_node_process (write1);
  
  gegl_node_link_many (image, grey, blur2, write2, NULL);
  gegl_node_process (write2);
  
  GeglSampler *sampler1 = gegl_buffer_sampler_new (dest1,
                                                  babl_format ("Y float"),
                                                  GEGL_SAMPLER_CUBIC);
  GeglSampler *sampler2 = gegl_buffer_sampler_new (dest2,
                                                  babl_format ("Y float"),
                                                  GEGL_SAMPLER_CUBIC);
  gint n_pixels = whole_region->width * whole_region->height;
  gint    hist[2000];
  gdouble diff;
  gint    count;
  gint    sum;

  memset (hist, 0, sizeof (int) * 2000);
  count = 0;
  gfloat pixel1, pixel2;
  gint x = whole_region->x;
  gint y = whole_region->y;
  gint i;
  while (n_pixels--)
    {
      gegl_sampler_get (sampler1,
			x,
                        y,
                        NULL,
                        &pixel1);

      gegl_sampler_get (sampler2,
                        x,
                        y,
                        NULL,
                        &pixel2);
      diff = pixel1/pixel2;
      if (under_threshold)
	{
	  if (diff < THRESHOLD)
	    {
	      hist[(int) (diff * 1000)] += 1;
	      count += 1;
	    }
	}
      else 
	{
	  if (diff >= THRESHOLD && diff < 2.0)
	    {
	      hist[(int) (diff * 1000)] += 1;
	      count += 1;
	    }
	}
      /* update x and y coordinates */
      x++;
      if (x>=whole_region->x + whole_region->width)
        {
          x=whole_region->x;
          y++;
        }
    }

  g_object_unref (sampler1);
  g_object_unref (sampler2);

  g_object_unref (gegl);
  g_object_unref (dest1);
  g_object_unref (dest2);
  
  if (pct == 0.0 || count == 0)
    return (under_threshold ? 1.0 : 0.0);

  sum = 0;
  for (i = 0; i < 2000; i++)
    {
      sum += hist[i];
      if (((gdouble) sum / (gdouble) count) > pct)
        {
          if (under_threshold)
            return (THRESHOLD - (gdouble) i / 1000.0);
          else
            return ((gdouble) i / 1000.0 - THRESHOLD);
        }
    }
  
  return (under_threshold ? 0.0 : 1.0);
}

static void prepare (GeglOperation *operation)
{
  GeglChantO              *o;
    
  o       = GEGL_CHANT_PROPERTIES (operation);
  
  gegl_operation_set_format (operation, "input",
                             babl_format ("Y float"));
  gegl_operation_set_format (operation, "output",
                             babl_format ("Y float"));
  if(o->chant_data)
    {
      g_slice_free (Ramps, o->chant_data);
    }
  o->chant_data = NULL;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglChantO *o                    = GEGL_CHANT_PROPERTIES (operation);

  GeglNode *gegl, *image, *write1, *write2, *grey, *blur1, *blur2;
  GeglBuffer *dest1 = gegl_buffer_new(result, babl_format ("Y float"));
  GeglBuffer *dest2 = gegl_buffer_new(result, babl_format ("Y float"));

  Ramps* ramps;
  
  // needs mutex
  if(o->chant_data == NULL) {
    o->chant_data = g_slice_new (Ramps);
    
    ramps = (Ramps*) o->chant_data;
    ramps->black = compute_ramp(input,operation,o->black,1);
    ramps->white = compute_ramp(input,operation,1.0-o->white,0);
  }
  
  gegl = gegl_node_new();
  image = gegl_node_new_child(gegl,
			      "operation", "gegl:buffer-source",
			      "buffer", input,
			      NULL);

  grey = gegl_node_new_child(gegl,
			     "operation", "gegl:grey",
			     NULL);

  blur1 =  gegl_node_new_child(gegl,
			       "operation", "gegl:gaussian-blur",
			       "std_dev_x", (MAX(1.0, 10*(1.0 - o->sharpness)) + 1.0)/sqrt(2.0),
			       "std_dev_y", (MAX(1.0, 10*(1.0 - o->sharpness)) + 1.0)/sqrt(2.0),
			       NULL);
  
  blur2 =  gegl_node_new_child(gegl,
			       "operation", "gegl:gaussian-blur",
			       "std_dev_x", (o->mask_radius + 1.0)/sqrt(2.0),
			       "std_dev_y", (o->mask_radius + 1.0)/sqrt(2.0),
			       NULL);
  
  write1 = gegl_node_new_child(gegl,
			      "operation", "gegl:buffer-sink",
			      "buffer", &dest1, NULL);
  
  write2 = gegl_node_new_child(gegl,
			      "operation", "gegl:buffer-sink",
			      "buffer", &dest2, NULL);
  

  gegl_node_link_many (image, grey, blur1, write1, NULL);
  gegl_node_process (write1);
  
  gegl_node_link_many (image, grey, blur2, write2, NULL);
  gegl_node_process (write2);
  
  GeglSampler *sampler1 = gegl_buffer_sampler_new (dest1,
                                                  babl_format ("Y float"),
                                                  GEGL_SAMPLER_CUBIC);
  GeglSampler *sampler2 = gegl_buffer_sampler_new (dest2,
                                                  babl_format ("Y float"),
                                                  GEGL_SAMPLER_CUBIC);

  gfloat *dst_buf = g_slice_alloc (result->width * result->height * 1 * sizeof(gfloat));
  
  gint n_pixels = result->width * result->height;
  
  gfloat pixel1, pixel2;

  gfloat *out_pixel = dst_buf;

  gint x = result->x; /* initial x                   */
  gint y = result->y; /*           and y coordinates */

  gdouble diff;
  Ramps *get_ramps = (Ramps*) o->chant_data;
  gdouble ramp_down = get_ramps->black;
  gdouble ramp_up = get_ramps->white;
  gdouble mult;
  while (n_pixels--)
    {
      gegl_sampler_get (sampler1,
			x,
                        y,
                        NULL,
                        &pixel1);

      gegl_sampler_get (sampler2,
                        x,
                        y,
                        NULL,
                        &pixel2);
      diff = pixel1/pixel2;
      if (diff < THRESHOLD)
	{
	  if (ramp_down == 0.0)
	    mult = 0.0;
	  else
	    mult = (ramp_down - MIN (ramp_down,
				     (THRESHOLD - diff))) / ramp_down;
	  *out_pixel = pixel1 * mult;
	} 
      else
	{
	  if (ramp_up == 0.0)
	    mult = 1.0;
	  else
	    mult = MIN (ramp_up,
			(diff - THRESHOLD)) / ramp_up;

	   *out_pixel = 1.0 - (1.0 - mult) * (1.0 - pixel1);
	}
      out_pixel += 1;
      
      /* update x and y coordinates */
      x++;
      if (x>=result->x + result->width)
        {
          x=result->x;
          y++;
        }
    }
  
  gegl_buffer_set (output, result, 0, babl_format ("Y float"), dst_buf, GEGL_AUTO_ROWSTRIDE);
  g_slice_free1 (result->width * result->height * 1 * sizeof(gfloat), dst_buf);
  g_object_unref (sampler1);
  g_object_unref (sampler2);

  g_object_unref (gegl);
  g_object_unref (dest1);
  g_object_unref (dest2);
  
  return  TRUE;
}

static void
finalize (GObject *object)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (object);

  if (o->chant_data)
    {
      g_slice_free (Ramps, o->chant_data);
      o->chant_data = NULL;
    }

  G_OBJECT_CLASS (gegl_chant_parent_class)->finalize (object);
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GObjectClass               *object_class;
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  
  object_class    = G_OBJECT_CLASS (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  object_class->finalize = finalize;
  operation_class->prepare = prepare;
  filter_class->process    = process;

  gegl_operation_class_set_keys (operation_class,
    "categories" , "artistic",
    "name"       , "gegl:photocopy",
    "description", _("Photocopy effect"),
    NULL);
}

#endif
