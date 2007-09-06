/* This file is part of GEGL.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Copyright 2006, 2007 Øyvind Kolås <pippin@gimp.org>
 */
#include <glib.h>
#include <glib-object.h>
#include <string.h>

#include <babl/babl.h>
#include "gegl-provider.h"
#include "gegl-tile-backend.h"

G_DEFINE_TYPE (GeglTileBackend, gegl_tile_backend, GEGL_TYPE_TILE_STORE)
static GObjectClass * parent_class = NULL;

enum
{
  PROP_0,
  PROP_TILE_WIDTH,
  PROP_TILE_HEIGHT,
  PROP_PX_SIZE,
  PROP_TILE_SIZE,
  PROP_FORMAT
};

static void
get_property (GObject    *gobject,
              guint       property_id,
              GValue     *value,
              GParamSpec *pspec)
{
  GeglTileBackend *backend = GEGL_TILE_BACKEND (gobject);

  switch (property_id)
    {
      case PROP_TILE_WIDTH:
        g_value_set_int (value, backend->tile_width);
        break;

      case PROP_TILE_HEIGHT:
        g_value_set_int (value, backend->tile_height);
        break;

      case PROP_TILE_SIZE:
        g_value_set_int (value, backend->tile_size);
        break;

      case PROP_PX_SIZE:
        g_value_set_int (value, backend->px_size);
        break;

      case PROP_FORMAT:
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

static void
set_property (GObject      *gobject,
              guint         property_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglTileBackend *backend = GEGL_TILE_BACKEND (gobject);

  switch (property_id)
    {
      case PROP_TILE_WIDTH:
        backend->tile_width = g_value_get_int (value);
        return;

      case PROP_TILE_HEIGHT:
        backend->tile_height = g_value_get_int (value);
        return;

      case PROP_FORMAT:
        backend->format = g_value_get_pointer (value);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

static GObject *
constructor (GType                  type,
             guint                  n_params,
             GObjectConstructParam *params)
{
  GObject         *object;
  GeglTileBackend *backend;

  object  = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);
  backend = GEGL_TILE_BACKEND (object);

  g_assert (backend->tile_width > 0 &&
            backend->tile_height > 0);

  g_assert (backend->format);
  backend->px_size = ((Babl *) (backend->format))->format.bytes_per_pixel;

  backend->tile_size = backend->tile_width * backend->tile_height * backend->px_size;

  return object;
}


static void
gegl_tile_backend_class_init (GeglTileBackendClass *klass)
{
  GObjectClass *gobject_class;

  parent_class  = g_type_class_peek_parent (klass);
  gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
  gobject_class->constructor  = constructor;

  g_object_class_install_property (gobject_class, PROP_TILE_WIDTH,
                                   g_param_spec_int ("tile-width", "tile-width", "Tile width in pixels",
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
  g_object_class_install_property (gobject_class, PROP_TILE_HEIGHT,
                                   g_param_spec_int ("tile-height", "tile-height", "Tile height in pixels",
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
  g_object_class_install_property (gobject_class, PROP_TILE_SIZE,
                                   g_param_spec_int ("tile-size", "tile-size", "Size of the tiles linear buffer in bytes",
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READABLE));
  g_object_class_install_property (gobject_class, PROP_PX_SIZE,
                                   g_param_spec_int ("px-size", "px-size", "Size of a single pixel in bytes",
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READABLE));
  g_object_class_install_property (gobject_class, PROP_FORMAT,
                                   g_param_spec_pointer ("format", "format", "babl format",
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
}

static void
gegl_tile_backend_init (GeglTileBackend *self)
{
}
