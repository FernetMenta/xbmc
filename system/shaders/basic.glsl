/*
 *      Copyright (C) 2010-2015 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

uniform sampler2D img;
varying vec2 cord;

void main()
{
  gl_FragColor.rgb = texture2D(img, cord).rgb;
#if (TO_FULL_RANGE)
  gl_FragColor.rgb = (gl_FragColor.rgb  -0.0627) * 1.1644;
#endif
  gl_FragColor.a = gl_Color.a;
}

