/**
 * @file
 * @author  Chrisitan Urich <christian.urich@gmail.com>
 * @version 1.0
 * @section LICENSE
 *
 * This file is part of VIBe2
 *
 * Copyright (C) 2011  Christian Urich

 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */
//
// C++ Implementation: position
//
// Description: 
//
//
// Author: Christian Urich <christian.urich@gmail.com>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "csg_s_position.h"
namespace csg_s {
Position::Position() {
        this->height = 0;
        this->width = 0;
};

Position::Position(long width, long height)
{
	this->height = height;
	this->width = width;
}


Position::~Position()
{
}


/**
*Geter and Seter
**/
long Position::getHeight(){
	return this->height;
}

long Position::getWidth(){
	return this->width;
}

}
