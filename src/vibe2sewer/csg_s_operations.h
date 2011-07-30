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
#ifndef CSG_SCSG_S_OPERATIONS_H
#define CSG_SCSG_S_OPERATIONS_H
#include "compilersettings.h"
#include "dataLayer.h"

#include <iostream>
#include <string>

using namespace std;

namespace csg_s {

/**@brief Allgmeine Funktionen
 * @author Christian Urich <christian.urich@uibk.ac.at>
*/
class VIBE_HELPER_DLL_EXPORT csg_s_operations{
    
public:
    //csg_s_operations();

    //~csg_s_operations();
    ///rechnet die Position der Moore Nachbarschaft in X-Koordinate um
    static int returnPositionX(int Position);
    ///rechnet die Position der Moore Nachbarschaft in Y-Koordinate um
    static int returnPositionY(int Position);
    /// sucht nach einem Sammler und überträgt das Ergebniss in eine andere Landschaft
    static void findSewer(VIBe::dataLayer *Input, VIBe::dataLayer *Output);
    ///fügt ein nach außenhin abnehmendes radiales Feld ausgehend vom Standpunkt ein
    static void addRadiusValue(int x, int y, VIBe::dataLayer *Layer, int rmax);
    /**@brief fügt ein nach außenhin abnehmendes radiales Feld ausgehend vom Standpunkt ein
      *@param [in] value Multiplikator für das radiale Feld ersetzend hinzugefügt
      */
    static void addRadiusValue(int x, int y, VIBe::dataLayer *Layer, int rmax, double value);
    /**@brief fügt ein nach außenhin abnehmendes radiales Feld ausgehend vom Standpunkt ein
      *@param [in] value Multiplikator für das radiale Feld errodierend hinzugefügt
      */
    static void addRadiusValueADD(int x, int y, VIBe::dataLayer *Layer, int rmax, double value);
    /**@brief erzeugt ein Anziehungsfeld um das Initiale Kanalsystem
      */
    static void intSewerNetwork(VIBe::dataLayer *InitSewer, VIBe::dataLayer *ResultField);
    ///erportiert die Landschaft in ein *grd File
    static void exportRaster(QString name, VIBe::dataLayer * Layer_, double multiplikator);
    static void writePath(string Text, QString name);
};

}

#endif
