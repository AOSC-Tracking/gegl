#!/usr/bin/env ruby
# -*- coding: utf-8 -*-

copyright = '
/* !!!! AUTOGENERATED FILE generated by math.rb !!!!!
 *
 * This file is an image processing operation for GEGL
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
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 *
 * !!!! AUTOGENERATED FILE !!!!!
 */'

a = [
      ['add',       'result = input + value', 0.0],
      ['subtract',  'result = input - value', 0.0],
      ['multiply',  'result = input * value', 1.0],
      ['divide',    'result = value==0.0f?0.0f:input/value', 1.0],
      ['gamma',     'result = powf (input, value)', 1.0],
#     ['threshold', 'result = c>=value?1.0f:0.0f', 0.5],
#     ['invert',    'result = 1.0-c']
    ]

a.each do
    |item|

    name     = item[0] + ''
    filename = name + '.c'

    puts "generating #{filename}"
    file = File.open(filename, 'w')

    name        = item[0]
    capitalized = name.capitalize
    swapcased   = name.swapcase
    formula     = item[1]

    file.write copyright
    file.write "
#include \"config.h\"
#include <glib/gi18n-lib.h>


#ifdef GEGL_PROPERTIES

property_double (value, _(\"Value\"), #{item[2]})
   description(_(\"global value used if aux doesn't contain data\"))
   ui_range (-1.0, 1.0)

#else

#define GEGL_OP_POINT_COMPOSER
#define GEGL_OP_NAME         #{name}
#define GEGL_OP_C_FILE       \"#{filename}\"

#include \"gegl-op.h\"

#include <math.h>
#ifdef _MSC_VER
#define powf(a,b) ((gfloat)pow(a,b))
#endif


static void prepare (GeglOperation *operation)
{
  const Babl *format = babl_format (\"RGBA float\");

  gegl_operation_set_format (operation, \"input\", format);
  gegl_operation_set_format (operation, \"aux\", babl_format (\"RGB float\"));
  gegl_operation_set_format (operation, \"output\", format);
}

static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *aux_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  gfloat * GEGL_ALIGNED in = in_buf;
  gfloat * GEGL_ALIGNED out = out_buf;
  gfloat * GEGL_ALIGNED aux = aux_buf;
  gint    i;

  if (aux == NULL)
    {
      gfloat value = GEGL_PROPERTIES (op)->value;
      for (i=0; i<n_pixels; i++)
        {
          gint   j;
          for (j=0; j<3; j++)
            {
              gfloat result;
              gfloat input=in[j];
              #{formula};
              out[j]=result;
            }
          out[3]=in[3];
          in += 4;
          out+= 4;
        }
    }
  else
    {
      for (i=0; i<n_pixels; i++)
        {
          gint   j;
          gfloat value;
          for (j=0; j<3; j++)
            {
              gfloat input =in[j];
              gfloat result;
              value=aux[j];
              #{formula};
              out[j]=result;
            }
          out[3]=in[3];
          in += 4;
          aux += 3;
          out+= 4;
        }
    }

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass              *operation_class;
  GeglOperationPointComposerClass *point_composer_class;

  operation_class  = GEGL_OPERATION_CLASS (klass);
  point_composer_class     = GEGL_OPERATION_POINT_COMPOSER_CLASS (klass);

  point_composer_class->process = process;
  operation_class->prepare = prepare;

  gegl_operation_class_set_keys (operation_class,
  \"name\"        , \"gegl:#{name}\",
  \"title\"       , \"#{name.capitalize}\",
  \"categories\"  , \"compositors:math\",
  \"description\" ,
       _(\"Math operation #{name}, performs the operation per pixel, using either the constant provided in 'value' or the corresponding pixel from the buffer on aux as operands. (formula: #{formula})\"),
       NULL);
}
#endif
"
    file.close
end
