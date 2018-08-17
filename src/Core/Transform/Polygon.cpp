#include <Transform/Polygon.hpp>
#include <Utils/VectorUtils.hpp>
#include "Utils/MathUtils.hpp"

namespace obe::Transform
{
    PolygonPoint::PolygonPoint(Polygon* parent, unsigned index) : 
        m_parent(parent),
        index(index)
    {
    }

    PolygonPoint::PolygonPoint(Polygon* parent, unsigned index, const Transform::UnitVector& position) : 
        PolygonPoint(parent, index)
    {
        this->x = position.x;
        this->y = position.y;
    }

    void PolygonPoint::remove() const
    {
        m_parent->m_points.erase(m_parent->m_points.begin() + index);
    }

    double PolygonPoint::distance(const Transform::UnitVector& position) const
    {
        const Transform::UnitVector pVec = position.to<Transform::Units::WorldUnits>();
        return std::sqrt(std::pow((pVec.x - x), 2) + std::pow((pVec.y - y), 2));
    }

    void PolygonPoint::setRelativePosition(RelativePositionFrom from, const Transform::UnitVector& position)
    {
        if (from == RelativePositionFrom::Point0)
            this->set(position.to<Transform::Units::WorldUnits>() + m_parent->get(0));
        else if (from == RelativePositionFrom::Centroid)
        {
            const Transform::UnitVector centroid = m_parent->getCentroid();
            this->set(position.to<Transform::Units::WorldUnits>() + centroid);
        }
    }

    void PolygonPoint::move(const Transform::UnitVector& position)
    {
        this->add(position);
    }

    Polygon::Polygon() :
        Movable(Transform::MovableType::Polygon)
    {
    }

    void Polygon::resetUnit(Transform::Units unit)
    {
    }

    unsigned int Polygon::getPointsAmount() const
    {
        return m_points.size();
    }

    void Polygon::addPoint(const Transform::UnitVector& position, int pointIndex)
    {
        const Transform::UnitVector pVec = position.to<Transform::Units::WorldUnits>();
        if (pointIndex == -1 || pointIndex == m_points.size())
            m_points.push_back(std::make_unique<PolygonPoint>(this, pointIndex, pVec));
        else if (pointIndex >= 0 && pointIndex < m_points.size())
            m_points.insert(m_points.begin() + pointIndex, std::make_unique<PolygonPoint>(this, pointIndex, pVec));
    }

    PolygonSegment Polygon::getLine(point_index_t index)
    {
        if (index < m_points.size() - 1)
            return std::pair<PolygonPoint&, PolygonPoint&>(*m_points[index], *m_points[index + 1]);
        else if (index == m_points.size() - 1)
            return std::pair<PolygonPoint&, PolygonPoint&>(*m_points[index], *m_points[0]);
        else
            throw aube::ErrorHandler::Raise("obe.Transform.Polygon.IndexOverflow", { {"method", "getLine"}, {"index", std::to_string(index) } });
    }

    PolygonPoint& Polygon::findClosestPoint(const Transform::UnitVector& position, bool neighbor, const std::vector<point_index_t>& excludedPoints)
    {
        if (!m_points.empty())
        {
            const Transform::UnitVector pVec = position.to<Transform::Units::WorldUnits>();
            int closestPoint = 0;
            double tiniestDist = -1;
            for (unsigned int i = 0; i < m_points.size(); i++)
            {
                const double currentPointDist = m_points[i]->distance(pVec);
                if ((tiniestDist == -1 || tiniestDist > currentPointDist) && !Utils::Vector::contains(i, excludedPoints))
                {
                    closestPoint = i;
                    tiniestDist = currentPointDist;
                }
            }
            if (neighbor)
            {
                int leftNeighbor = closestPoint - 1;
                int rightNeighbor = closestPoint + 1;
                if (leftNeighbor < 0)
                    leftNeighbor = m_points.size() - 1;
                if (rightNeighbor >= m_points.size())
                    rightNeighbor = 0;
                const int leftNeighborDist = m_points[leftNeighbor]->distance(position);
                const int rightNeighborDist = m_points[rightNeighbor]->distance(position);
                if (leftNeighborDist > rightNeighborDist)
                {
                    closestPoint++;
                    if (closestPoint >= m_points.size())
                        closestPoint = 0;
                }
            }
            return *m_points[closestPoint];
        }
        throw aube::ErrorHandler::Raise("ObEngine.Collision.PolygonalCollider.NotEnoughPoints",
            { { "points", std::to_string(m_points.size()) }, { "function", "findClosestPoint" } });
    }

    std::pair<PolygonPoint&, PolygonPoint&> Polygon::findClosestLine(const Transform::UnitVector& position)
    {
        const Transform::UnitVector p3 = position.to<Transform::Units::WorldUnits>();
        const auto distanceLineFromPoint = [](const Transform::UnitVector& point, const Transform::UnitVector& lineP1, const Transform::UnitVector& lineP2)
        {
            Transform::UnitVector lineDiff = lineP2 - lineP1;
            if (lineDiff.x == 0 && lineDiff.y == 0)
            {
                lineDiff = point - lineP1;
                return sqrt(lineDiff.x * lineDiff.x + lineDiff.y * lineDiff.y);
            }

            double t = ((point.x - lineP1.x) * lineDiff.x + (point.y - lineP1.y) * lineDiff.y) / (lineDiff.x * lineDiff.x + lineDiff.y * lineDiff.y);

            if (t < 0)
            {
                //point is nearest to the first point i.e x1 and y1
                lineDiff = point - lineP1;
            }
            else if (t > 1)
            {
                //point is nearest to the end point i.e x2 and y2
                lineDiff = point - lineP2;
            }
            else
            {
                //if perpendicular line intersect the line segment.
                lineDiff.x = point.x - (lineP1.x + t * lineDiff.x);
                lineDiff.y = point.y - (lineP1.y + t * lineDiff.y);
            }

            //returning shortest distance
            return sqrt(lineDiff.x * lineDiff.x + lineDiff.y * lineDiff.y);
        };
        double shortestDistance = -1;
        unsigned int shortestIndex = 0;
        for (unsigned int i = 0, j = getAllPoints().size() - 1; i < getAllPoints().size(); j = i++)
        {
            const double currentDistance = distanceLineFromPoint(p3, this->get(i), this->get(j));
            if (shortestDistance == -1 || currentDistance < shortestDistance)
            {
                shortestDistance = currentDistance;
                shortestIndex = i;
            }
        }
        return this->getLine(shortestIndex);
    }

    std::optional<PolygonSegment> Polygon::getSegmentContainingPoint(const Transform::UnitVector& position, const double tolerance)
    {
        for (point_index_t i = 0; i < m_points.size(); i++)
        {
            const point_index_t nextNode = (i != m_points.size() - 1) ? i + 1 : 0;
            const double lineLength = m_points[i]->distance(this->get(nextNode));
            const double firstLength = m_points[i]->distance(position);
            const double secondLength = m_points[nextNode]->distance(position);
            if (Utils::Math::isBetween(lineLength, firstLength + secondLength - tolerance, firstLength + secondLength + tolerance))
                return std::make_optional(this->getLine(i));
        }
        return std::nullopt;
    }

    PolygonPath& Polygon::getAllPoints()
    {
        return m_points;
    }

    Transform::UnitVector Polygon::getCentroid() const
    {
        Transform::UnitVector centroid = { 0, 0 };
        double signedArea = 0.0;
        double x0, y0, x1, y1, a;

        int i;
        for (i = 0; i < m_points.size() - 1; ++i)
        {
            x0 = m_points[i]->x;
            y0 = m_points[i]->y;
            x1 = m_points[i + 1]->x;
            y1 = m_points[i + 1]->y;
            a = x0 * y1 - x1 * y0;
            signedArea += a;
            centroid.x += (x0 + x1) * a;
            centroid.y += (y0 + y1) * a;
        }

        x0 = m_points[i]->x;
        y0 = m_points[i]->y;
        x1 = m_points[0]->x;
        y1 = m_points[0]->y;
        a = x0 * y1 - x1 * y0;
        signedArea += a;
        centroid.x += (x0 + x1) * a;
        centroid.y += (y0 + y1) * a;

        signedArea *= 0.5;
        centroid.x /= (6.0 * signedArea);
        centroid.y /= (6.0 * signedArea);

        return centroid;
    }

    std::optional<PolygonPoint*> Polygon::getPointAroundPosition(const Transform::UnitVector& position, const Transform::UnitVector& tolerance)
    {
        const Transform::UnitVector pVec = position.to<Transform::Units::WorldUnits>();
        const Transform::UnitVector pTolerance = tolerance.to<Transform::Units::WorldUnits>();
        for (auto& m_point : m_points)
        {
            if (Utils::Math::isBetween(pVec.x, m_point->x - pTolerance.x, m_point->x + pTolerance.x))
            {
                if (Utils::Math::isBetween(pVec.y, m_point->y - pTolerance.y, m_point->y + pTolerance.y))
                    return std::optional<PolygonPoint*>(m_point.get());
            }
        }
        return std::nullopt;
    }

    bool Polygon::isCentroidAroundPosition(const Transform::UnitVector& position, const Transform::UnitVector& tolerance) const
    {
        const Transform::UnitVector pVec = position.to<Transform::Units::WorldUnits>();
        const Transform::UnitVector pTolerance = tolerance.to<Transform::Units::WorldUnits>();
        const Transform::UnitVector centroid = this->getCentroid();
        if (Utils::Math::isBetween(pVec.x, centroid.x - pTolerance.x, centroid.x + pTolerance.x))
        {
            if (Utils::Math::isBetween(pVec.y, centroid.y - pTolerance.x, centroid.y + pTolerance.y))
                return true;
        }
        return false;
    }

    double Polygon::getSegmentAngle(const point_index_t segment)
    {
        int p2 = segment + 1;
        if (segment == m_points.size() - 1)
            p2 = 0;
        const auto p1c = this->get(segment);
        const auto p2c = this->get(p2);
        const double deltaX = p2c.x - p1c.x;
        const double deltaY = p2c.y - p1c.y;
        return (std::atan2(deltaY, deltaX) * 180.0 / Utils::Math::pi);
    }

    double Polygon::getSegmentLength(const point_index_t segment)
    {
        int p2 = segment + 1;
        if (segment == m_points.size() - 1)
            p2 = 0;
        return m_points[segment]->distance(this->get(p2));
    }

    UnitVector Polygon::getPosition() const
    {
        return static_cast<UnitVector>(*m_points[0]);
    }


    void Polygon::setRotation(const float angle, const Transform::UnitVector origin)
    {
        this->rotate(angle - m_angle, origin);
    }

    float Polygon::getRotation() const
    {
        return m_angle;
    }

    void Polygon::rotate(float angle, Transform::UnitVector origin)
    {
        m_angle += angle;

        const double radAngle = (Utils::Math::pi / 180.0) * -angle;
        for (auto& point : m_points)
        {
            point->set(
                std::cos(radAngle) * (point->x - origin.x) - std::sin(radAngle) * (point->y - origin.y) + origin.x,
                std::sin(radAngle) * (point->x - origin.x) + std::cos(radAngle) * (point->y - origin.y) + origin.y
            );
        }
    }

    void Polygon::move(const Transform::UnitVector& position)
    {
        if (!m_points.empty())
        {
            for (auto& point : m_points)
                *point += position;
        }
    }

    void Polygon::setPosition(const Transform::UnitVector& position)
    {
        if (!m_points.empty())
        {
            const Transform::UnitVector pVec = position.to<Transform::Units::WorldUnits>();
            const Transform::UnitVector addPosition = pVec - *m_points[0];

            m_points[0]->set(pVec);
            for (int i = 1; i < m_points.size(); i++)
            {
                *m_points[i] += addPosition;
            }
        }
    }

    void Polygon::setPositionFromCentroid(const Transform::UnitVector& position)
    {
        if (!m_points.empty())
        {
            const Transform::UnitVector pVec = position.to<Transform::Units::WorldUnits>();
            const Transform::UnitVector centroid = this->getCentroid();
            const Transform::UnitVector addPosition = pVec - centroid;

            for (auto& point : m_points)
            {
                *point += addPosition;
            }
        }
    }

    PolygonPoint& Polygon::operator[](point_index_t i)
    {
        return this->get(i);
    }

    PolygonPoint& Polygon::get(point_index_t i)
    {
        if (i < m_points.size())
            return *m_points[i];
        else
            throw aube::ErrorHandler::Raise("obe.Transform.Polygon.IndexOverflow", { { "method", "operator[]" },{ "index", std::to_string(i) } });
    }
}
