/*
  Copyright 2020 Bundesamt fuer Sicherheit in der Informationstechnik (BSI)

  This file is part of RdpCacheStitcher.

  RdpCacheStitcher is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  RdpCacheStitcher is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with RdpCacheStitcher.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef TILE_H
#define TILE_H

#include <QString>
#include <QImage>
#include <QVector>
#include <QHash>
#include <QColor>

class Tile
{
public:
    enum Edge {Top, Right, Bottom, Left};
    enum Filter {Gauss1, Gauss6, Gauss15};
    //
    // Helper struct to store fractional RGB values
    //
    struct AvgColor {
        double red;
        double green;
        double blue;

        AvgColor() : red(0), green(0), blue(0) {}
        AvgColor(double red, double green, double blue) : red(red), green(green), blue(blue) {}
    };
    //
    // Width and height of a tile are identical.
    // If height < width, height is set to width.
    // If width < height, tile is invalid.
    //
    int size = 0;
    static const int MAX_SIZE = 256;
    //
    // Indicate if original height was smaller than width
    //
    bool isResized = false;
    bool isDuplicate = false;

    Tile(QString filename);
    Tile(QImage image, bool isResized, bool isDuplicate);

    bool isNull() const;
    Edge oppositeEdge(Edge e);
    QImage getImage() const;
    QVector<AvgColor> getEdgeColors(Edge edge, Filter filter);
    double calcEdgeSimilarity(Tile &other, Filter filter, Edge edge);
    int getNumUniqueEdgeColors(Edge edge, Filter filter);

private:
    static const int Gauss1Kernel[5][5];
    static const int Gauss6Kernel[5][5];
    static const int Gauss15Kernel[5][5];

    QImage image;
    //
    // Arrays storing gaussian-filtered color values for all edges and filters
    //
    QHash<Filter, QHash<Edge, QVector<AvgColor>>> edgeColorsByFilter;
    //
    // Number of unique colors per edge
    //
    QHash<Filter, QHash<Edge, int>> numUniqueEdgeColors;

    int calcNumUniqueEdgeColors(Edge edge, Filter filter);
    void precalcEdgeColors();
    QVector<AvgColor> edgeColors(int startX, int startY, int deltaX, int deltaY, Filter filter);
    int getGaussKernelValue(int x, int y, Filter filter);
};

#endif // TILE_H
