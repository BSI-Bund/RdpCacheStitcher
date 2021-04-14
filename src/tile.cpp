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

#include <QImage>
#include <iostream>
#include <math.h>
#include <stdexcept>
#include <QSet>

#include "tile.h"

const int Tile::Gauss1Kernel[5][5] = {
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 256, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0}
};
const int Tile::Gauss6Kernel[5][5] = {
    {0, 0, 0, 0, 0},
    {0, 16, 32, 16, 0},
    {0, 32, 64, 32, 0},
    {0, 16, 32, 16, 0},
    {0, 0, 0, 0, 0}
};
const int Tile::Gauss15Kernel[5][5] = {
    {1, 4, 6, 4, 1},
    {4, 16, 24, 16, 4},
    {6, 24, 36, 24, 6},
    {4, 16, 24, 16, 4},
    {1, 4, 6, 4, 1}
};

Tile::Tile(QString filename)
{
    image.load(filename);
    if (!image.isNull()) {
        if (image.width() > MAX_SIZE || image.height() > image.width()) {
            // Invalidate image
            image.loadFromData(NULL, 0);
        } else {
            if (image.height() < image.width()) {
                isResized = true;
                image.scaled(image.width(), image.width(), Qt::KeepAspectRatio);
            }
            size = image.width();
            precalcEdgeColors();
        }
    }
}

Tile::Tile(QImage image, bool isResized, bool isDuplicate)
{
    this->image = image;
    size = image.width();
    this->isResized = isResized;
    this->isDuplicate = isDuplicate;
    precalcEdgeColors();
}

bool Tile::isNull() const
{
    return image.isNull();
}

Tile::Edge Tile::oppositeEdge(Tile::Edge e)
{
    switch (e) {
        case Top:
            return Bottom;
        case Right:
            return Left;
        case Bottom:
            return Top;
        case Left:
            return Right;
    }
    throw std::runtime_error("Invalid edge type!");
}

QImage Tile::getImage() const
{
    return image;
}

QVector<Tile::AvgColor> Tile::getEdgeColors(Edge edge, Tile::Filter filter)
{
    return edgeColorsByFilter.value(filter).value(edge);
}

double Tile::calcEdgeSimilarity(Tile &other, Tile::Filter filter, Tile::Edge edge)
{
    double error = 0.0;
    QVector<AvgColor> ownColors = getEdgeColors(edge, filter);
    QVector<AvgColor> otherColors = other.getEdgeColors(oppositeEdge(edge), filter);
    for (int i = 0; i < size; ++i) {
        const AvgColor &ownColor = ownColors.at(i);
        const AvgColor &otherColor = otherColors.at(i);
        const double redDiff = ownColor.red - otherColor.red;
        const double greenDiff = ownColor.green - otherColor.green;
        const double blueDiff = ownColor.blue - otherColor.blue;
        error += sqrt(redDiff*redDiff + greenDiff*greenDiff + blueDiff*blueDiff);
    }
    double normalized = 1.0 - (error / sqrt(255*255*3) / size);
    return normalized;
}

int Tile::getNumUniqueEdgeColors(Tile::Edge edge, Tile::Filter filter)
{
    return numUniqueEdgeColors.value(filter).value(edge);
}

int Tile::calcNumUniqueEdgeColors(Tile::Edge edge, Tile::Filter filter)
{
    QSet<unsigned int> edgeColorsHistogram;
    QVector<AvgColor> edgeColors = getEdgeColors(edge, filter);
    for (int i = 0; i < edgeColors.size(); ++i) {
        AvgColor c = edgeColors.at(i);
        unsigned int rounded = uint(c.red) + 256*uint(c.green) + 256*256*uint(c.blue);
        edgeColorsHistogram.insert(rounded);
    }
    return edgeColorsHistogram.size();
}

void Tile::precalcEdgeColors()
{
    QHash<Edge, QVector<AvgColor>> gauss1Edges;
    gauss1Edges.insert(Edge::Top, edgeColors(0, 0, 1, 0, Filter::Gauss1));
    gauss1Edges.insert(Edge::Right, edgeColors(size - 1, 0, 0, 1, Filter::Gauss1));
    gauss1Edges.insert(Edge::Bottom, edgeColors(0, size - 1, 1, 0, Filter::Gauss1));
    gauss1Edges.insert(Edge::Left, edgeColors(0, 0, 0, 1, Filter::Gauss1));
    edgeColorsByFilter.insert(Filter::Gauss1, gauss1Edges);
    QHash<Edge, QVector<AvgColor>> gauss6Edges;
    gauss6Edges.insert(Edge::Top, edgeColors(0, 0, 1, 0, Filter::Gauss6));
    gauss6Edges.insert(Edge::Right, edgeColors(size - 1, 0, 0, 1, Filter::Gauss6));
    gauss6Edges.insert(Edge::Bottom, edgeColors(0, size - 1, 1, 0, Filter::Gauss6));
    gauss6Edges.insert(Edge::Left, edgeColors(0, 0, 0, 1, Filter::Gauss6));
    edgeColorsByFilter.insert(Filter::Gauss6, gauss6Edges);
    QHash<Edge, QVector<AvgColor>> gauss15Edges;
    gauss15Edges.insert(Edge::Top, edgeColors(0, 0, 1, 0, Filter::Gauss15));
    gauss15Edges.insert(Edge::Right, edgeColors(size - 1, 0, 0, 1, Filter::Gauss15));
    gauss15Edges.insert(Edge::Bottom, edgeColors(0, size - 1, 1, 0, Filter::Gauss15));
    gauss15Edges.insert(Edge::Left, edgeColors(0, 0, 0, 1, Filter::Gauss15));
    edgeColorsByFilter.insert(Filter::Gauss15, gauss15Edges);
    // Unique colors per edge
    QHash<Edge, int> gauss1Uniques;
    gauss1Uniques.insert(Edge::Top, calcNumUniqueEdgeColors(Edge::Top, Filter::Gauss1));
    gauss1Uniques.insert(Edge::Right, calcNumUniqueEdgeColors(Edge::Right, Filter::Gauss1));
    gauss1Uniques.insert(Edge::Bottom, calcNumUniqueEdgeColors(Edge::Bottom, Filter::Gauss1));
    gauss1Uniques.insert(Edge::Left, calcNumUniqueEdgeColors(Edge::Left, Filter::Gauss1));
    numUniqueEdgeColors.insert(Filter::Gauss1, gauss1Uniques);
    QHash<Edge, int> gauss6Uniques;
    gauss6Uniques.insert(Edge::Top, calcNumUniqueEdgeColors(Edge::Top, Filter::Gauss6));
    gauss6Uniques.insert(Edge::Right, calcNumUniqueEdgeColors(Edge::Right, Filter::Gauss6));
    gauss6Uniques.insert(Edge::Bottom, calcNumUniqueEdgeColors(Edge::Bottom, Filter::Gauss6));
    gauss6Uniques.insert(Edge::Left, calcNumUniqueEdgeColors(Edge::Left, Filter::Gauss6));
    numUniqueEdgeColors.insert(Filter::Gauss6, gauss6Uniques);
    QHash<Edge, int> gauss15Uniques;
    gauss15Uniques.insert(Edge::Top, calcNumUniqueEdgeColors(Edge::Top, Filter::Gauss15));
    gauss15Uniques.insert(Edge::Right, calcNumUniqueEdgeColors(Edge::Right, Filter::Gauss15));
    gauss15Uniques.insert(Edge::Bottom, calcNumUniqueEdgeColors(Edge::Bottom, Filter::Gauss15));
    gauss15Uniques.insert(Edge::Left, calcNumUniqueEdgeColors(Edge::Left, Filter::Gauss15));
    numUniqueEdgeColors.insert(Filter::Gauss15, gauss15Uniques);
}

QVector<Tile::AvgColor> Tile::edgeColors(int startX, int startY, int deltaX, int deltaY, Tile::Filter filter)
{
    QVector<AvgColor> colors;
    int curX = startX;
    int curY = startY;
    for (int i = 0; i < size; ++i) {
        double divisor = 0;
        double red = 0;
        double green = 0;
        double blue = 0;
        for (int gaussY = 0; gaussY < 5; ++gaussY) {
            for (int gaussX = 0; gaussX < 5; ++gaussX) {
                if (image.valid(curX + gaussX - 2, curY + gaussY - 2)) {
                    const int gaussValue = getGaussKernelValue(gaussX, gaussY, filter);
                    divisor += gaussValue;
                    QColor pixel = image.pixelColor(curX + gaussX - 2, curY + gaussY - 2);
                    red += gaussValue*pixel.red();
                    green += gaussValue*pixel.green();
                    blue += gaussValue*pixel.blue();
                }
            }
        }
        colors.append(AvgColor(red/divisor, green/divisor, blue/divisor));
        curX += deltaX;
        curY += deltaY;
    }
    return colors;
}

int Tile::getGaussKernelValue(int x, int y, Filter filter)
{
    switch (filter) {
    case Filter::Gauss1:
        return Gauss1Kernel[x][y];
    case Filter::Gauss6:
        return Gauss6Kernel[x][y];
    case Filter::Gauss15:
        return Gauss15Kernel[x][y];
    default:
        throw std::runtime_error("Unsupported filter!");
    }
}
