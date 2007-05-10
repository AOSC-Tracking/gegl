/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2006 Øyvind Kolås
 */
#include "gegl-operation-point-filter.h"
#include <string.h>

static gboolean process_inner (GeglOperation *operation,
                               gpointer       context_id);

G_DEFINE_TYPE (GeglOperationPointFilter, gegl_operation_point_filter, GEGL_TYPE_OPERATION_FILTER)


static void
gegl_operation_point_filter_class_init (GeglOperationPointFilterClass *klass)
{
  GeglOperationFilterClass *filter_class = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process = process_inner;
}

static void
gegl_operation_point_filter_init (GeglOperationPointFilter *self)
{
  self->format = babl_format ("RGBA float");/* default to RGBA float for
                                                processing */
}

static gboolean
process_inner (GeglOperation *operation,
               gpointer       context_id)
{
  GeglOperationPointFilter *point_filter = GEGL_OPERATION_POINT_FILTER (operation);

  GeglBuffer               *input  = GEGL_BUFFER (gegl_operation_get_data (operation, context_id, "input"));
  GeglRectangle            *result = gegl_operation_result_rect (operation, context_id);
  GeglBuffer               *output;
  gfloat                   *buf;

  output = GEGL_BUFFER(gegl_operation_get_target (operation, context_id, "output"));

  if ((result->width > 0) && (result->height > 0))
    {
      buf = g_malloc (4 * sizeof (gfloat) * output->width * output->height);

      gegl_buffer_get (input, result, 1.0, point_filter->format, buf);

      GEGL_OPERATION_POINT_FILTER_GET_CLASS (operation)->process (
        operation,
        buf,
        buf,
        output->width * output->height);

      gegl_buffer_set (output, result, point_filter->format, buf);
      g_free (buf);
    }
  return TRUE;
}
