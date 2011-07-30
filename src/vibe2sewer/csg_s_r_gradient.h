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
#ifndef CSG_S__R_GRADIENT_H
#define CSG_S__R_GRADIENT_H
#include "compilersettings.h"
#include <QVector>
#include "dataLayer.h"
#include "csg_s_rule.h"
#include "csg_s_position.h"


namespace csg_s {
/**@brief Regel für die Rückgabe möglicher und Wahrscheinlicher Bewegungsrichtungen
 * @author Christian Urich <christian.urich@uibk.ac.at>
*/
class VIBE_HELPER_DLL_EXPORT R_Gradient:public rule {
private:
	QVector<int> possible_runoffField(Position* pos, double rel_height);
	double depth;
public:
        R_Gradient(VIBe::dataLayer *layer_, double depth);
        R_Gradient(VIBe::dataLayer *layer_);
        //liefer tiefste Zelle zurück
	double run(Position* pos);
        /**@brief ruft die Funktion possible_runoffField() auf
          *
          *@param [out] QVector<int> liefert die möglichen Bewegungsrichtungen zurück
          */
	QVector<int> run_field(Position* pos);
        /**@brief ruft die Funktion possible_runoffField() auf
          *
          *@param [out] QVector<int> liefert die möglichen Bewegungsrichtungen zurück
          */
	QVector<int> run_field(Position* pos, double rel_height);
        ///liefert alle Werte der Nachbarschaft zurück
	QVector<int> number_Field(Position* pos);
        //~R_Gradient();
};

}

#endif
