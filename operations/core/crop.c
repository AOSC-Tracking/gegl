/* This file is part of GEGL
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
 * License along with GEGL; if not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright 2006 Øyvind Kolås
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_PROPERTIES

property_double (x,      _("X"),      0.0)
  ui_range      (0.0, 1024.0)
  ui_meta       ("unit", "pixel-coordinate")
  ui_meta       ("axis", "x")

property_double (y,      _("Y"),      0.0)
  ui_range      (0.0, 1024.0)
  ui_meta       ("unit", "pixel-coordinate")
  ui_meta       ("axis", "y")

property_double (width,  _("Width"),  10.0 )
  ui_range      (0.0, 1024.0)
  ui_meta       ("unit", "pixel-distance")
  ui_meta       ("axis", "x")

property_double (height, _("Height"), 10.0 )
  ui_range      (0.0, 1024.0)
  ui_meta       ("unit", "pixel-distance")
  ui_meta       ("axis", "y")

property_boolean (reset_origin, _("Reset origin"), FALSE)

#else

#define GEGL_OP_COMPOSER
#define GEGL_OP_NAME     crop
#define GEGL_OP_C_SOURCE crop.c

#include "gegl-op.h"
#include <math.h>

static void
update_from_aux (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglRectangle *aux_rect = gegl_operation_source_get_bounding_box (operation, "aux");
  if (aux_rect)
  {
    o->x = aux_rect->x;
    o->y = aux_rect->y;
    o->width = aux_rect->width;
    o->height = aux_rect->height;
  }
}

static void
gegl_crop_prepare (GeglOperation *operation)
{
  const Babl *format = gegl_operation_get_source_format (operation, "input");

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);

  update_from_aux (operation);
}

static GeglNode *
gegl_crop_detect (GeglOperation *operation,
                  gint           x,
                  gint           y)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglNode   *input_node;

  update_from_aux (operation);
  input_node = gegl_operation_get_source_node (operation, "input");

  if (input_node)
    return gegl_node_detect (input_node,
                             x - floor (o->x),
                             y - floor (o->y));

  return operation->node;
}


static GeglRectangle
gegl_crop_get_bounding_box (GeglOperation *operation)
{
  GeglProperties      *o = GEGL_PROPERTIES (operation);
  GeglRectangle *in_rect = gegl_operation_source_get_bounding_box (operation, "input");
  GeglRectangle  result  = { 0, 0, 0, 0 };

  update_from_aux (operation);

  if (!in_rect)
    return result;

  result.x      = o->x;
  result.y      = o->y;
  result.width  = o->width;
  result.height = o->height;

  return result;
}

static GeglRectangle
gegl_crop_get_invalidated_by_change (GeglOperation       *operation,
                                     const gchar         *input_pad,
                                     const GeglRectangle *input_region)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglRectangle   result;
  update_from_aux (operation);

  result.x      = o->x;
  result.y      = o->y;
  result.width  = o->width;
  result.height = o->height;

  gegl_rectangle_intersect (&result, &result, input_region);

  return result;
}

static GeglRectangle
gegl_crop_get_required_for_output (GeglOperation       *operation,
                                   const gchar         *input_pad,
                                   const GeglRectangle *roi)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglRectangle   result;
  update_from_aux (operation);

  result.x      = o->x;
  result.y      = o->y;
  result.width  = o->width;
  result.height = o->height;

  gegl_rectangle_intersect (&result, &result, roi);
  return result;
}

static gboolean
gegl_crop_process (GeglOperation        *operation,
                   GeglOperationContext *context,
                   const gchar          *output_prop,
                   const GeglRectangle  *result,
                   gint                  level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglBuffer     *input;
  gboolean        success = FALSE;

  input = (GeglBuffer*) gegl_operation_context_dup_object (context, "input");

  if (input)
    {
      GeglRectangle  extent;
      GeglBuffer    *output;

      extent = *GEGL_RECTANGLE (o->x, o->y,  o->width, o->height);

      if (gegl_rectangle_equal (&extent, gegl_buffer_get_extent (input)))
        output = g_object_ref (input);
      else
        output = gegl_buffer_create_sub_buffer (input, &extent);

      /* propagate forked state, meaning that in-place processing is not possible
       * due to shared nature of underlying data
       */
      if (gegl_object_get_has_forked (G_OBJECT (input)))
        gegl_object_set_has_forked (G_OBJECT (output));

      gegl_operation_context_take_object (context, "output", G_OBJECT (output));

      g_object_unref (input);
      success = TRUE;
    }
  else
    {
      g_warning ("%s got NULL input pad", gegl_node_get_operation (operation->node));
    }

  return success;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class;
  gchar              *composition = "<?xml version='1.0' encoding='UTF-8'?>"
    "<gegl>"
    "<node operation='gegl:crop'>"
    "  <params>"
    "    <param name='x'>50</param>"
    "    <param name='y'>80</param>"
    "    <param name='width'>70</param>"
    "    <param name='height'>60</param>"
    "  </params>"
    "</node>"
    "<node operation='gegl:load'>"
    "  <params>"
    "    <param name='path'>standard-input.png</param>"
    "  </params>"
    "</node>"
    "</gegl>";

  operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->threaded                  = FALSE;
  operation_class->process                   = gegl_crop_process;
  operation_class->prepare                   = gegl_crop_prepare;
  operation_class->get_bounding_box          = gegl_crop_get_bounding_box;
  operation_class->detect                    = gegl_crop_detect;
  operation_class->get_invalidated_by_change = gegl_crop_get_invalidated_by_change;
  operation_class->get_required_for_output   = gegl_crop_get_required_for_output;

  gegl_operation_class_set_keys (operation_class,
      "name",        "gegl:crop",
      "categories",  "core",
      "title",       _("Crop"),
      "description", _("Crops a buffer, if the aux pad is connected the bounding box of the node connected is used."),
      "reference-hash", "6f9f160434a4e9484d334c29122e5682",
      "reference-composition", composition,
      NULL);

  operation_class->cache_policy = GEGL_CACHE_POLICY_NEVER;
}

#endif
