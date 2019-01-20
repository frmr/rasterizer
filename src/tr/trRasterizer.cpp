#include "trRasterizer.hpp"
#include "trCoord.hpp"
#include "trEdgeInfo.hpp"
#include "trVertexClipBitMasks.hpp"
#include <cassert>

void tr::Rasterizer::draw(std::vector<Vertex> vertices, const ColorBuffer& texture, ColorBuffer& colorBuffer, DepthBuffer& depthBuffer)
{
	if (colorBuffer.getWidth() == 0 || colorBuffer.getHeight() == 0)
	{
		return;
	}

	const float halfWidth  = float(colorBuffer.getWidth())  / 2.0f - 0.0001f;
	const float halfHeight = float(colorBuffer.getHeight()) / 2.0f - 0.0001f;

	for (auto& vertex : vertices)
	{
		vertex.position = m_matrix * vertex.position;
	}

	if (m_primitive == Primitive::TRIANGLES)
	{
		for (std::vector<Vertex>::const_iterator it = vertices.begin(); it < vertices.end() - 2; it += 3)
		{
			clipAndDrawTriangle({ *it, *(it + 1), *(it + 2) }, texture, halfWidth, halfHeight, colorBuffer, depthBuffer);
		}
	}
	else if (m_primitive == Primitive::TRIANGLE_STRIP)
	{

	}
	else if (m_primitive == Primitive::TRIANGLE_FAN)
	{

	}
}

void tr::Rasterizer::setPrimitive(const Primitive primitive)
{
	m_primitive = primitive;
}

void tr::Rasterizer::setMatrix(const Matrix4& matrix)
{
	m_matrix = matrix;
}

tr::Vertex tr::Rasterizer::lineFrustumIntersection(const Vertex& lineStart, const Vertex& lineEnd, const tr::Axis axis, const bool negativeW)
{
	const float   alpha        = negativeW ?
	                             (-lineStart.position.w  - lineStart.position[axis]) / (lineEnd.position[axis] - lineStart.position[axis] + lineEnd.position.w - lineStart.position.w) :
	                             ( lineStart.position.w  - lineStart.position[axis]) / (lineEnd.position[axis] - lineStart.position[axis] - lineEnd.position.w + lineStart.position.w);

	const Vector4 position     = lineStart.position     + (lineEnd.position     - lineStart.position)     * alpha;
	const Vector3 normal       = lineStart.normal       + (lineEnd.normal       - lineStart.normal)       * alpha;
	const Vector2 textureCoord = lineStart.textureCoord + (lineEnd.textureCoord - lineStart.textureCoord) * alpha;
	
	return { position, normal, textureCoord };
}

void tr::Rasterizer::drawTriangle(std::array<Vertex, 3> vertices, const ColorBuffer& texture, const float halfWidth, const float halfHeight, ColorBuffer& colorBuffer, DepthBuffer& depthBuffer)
{
	perspectiveDivide(vertices);

	if (orientPoint(vertices[0].position, vertices[1].position, vertices[2].position) >= 0.0f)
	{
		return;
	}

	viewportTransformation(vertices, halfWidth, halfHeight);

	const uint16_t minX = std::min({ uint16_t(vertices[0].position.x), uint16_t(vertices[1].position.x), uint16_t(vertices[2].position.x) });
	const uint16_t minY = std::min({ uint16_t(vertices[0].position.y), uint16_t(vertices[1].position.y), uint16_t(vertices[2].position.y) });
	const uint16_t maxX = std::max({ uint16_t(vertices[0].position.x), uint16_t(vertices[1].position.x), uint16_t(vertices[2].position.x) });
	const uint16_t maxY = std::max({ uint16_t(vertices[0].position.y), uint16_t(vertices[1].position.y), uint16_t(vertices[2].position.y) });

	pixelShift(vertices);
	sortVertices(vertices);
	fillTriangle(vertices, texture, colorBuffer, depthBuffer);
}

void tr::Rasterizer::clipAndDrawTriangle(const std::array<Vertex, 3>& vertices, const ColorBuffer& texture, const float halfWidth, const float halfHeight, ColorBuffer& colorBuffer, DepthBuffer& depthBuffer)
{
	std::array<unsigned char, 3> vertexClipBitFields     = { 0, 0, 0 };
	std::array<unsigned char, 3> vertexEqualityBitFields = { 0, 0, 0 };

	for (size_t vertexIndex = 0; vertexIndex < 3; ++vertexIndex)
	{
		const Vertex&   vertex           = vertices[vertexIndex];
		constexpr float margin           = 0.0001f;
		const float     wLessMargin      = vertex.position.w - margin;
		const float     wPlusMargin      = vertex.position.w + margin;
		unsigned char&  clipBitField     = vertexClipBitFields[vertexIndex];
		unsigned char&  equalityBitField = vertexEqualityBitFields[vertexIndex];

		if (vertex.position.x <  -wPlusMargin) { clipBitField     |= leftBitMask;                     }
		if (vertex.position.x >   wPlusMargin) { clipBitField     |= rightBitMask;                    }
		if (vertex.position.y <  -wPlusMargin) { clipBitField     |= bottomBitMask;                   }
		if (vertex.position.y >   wPlusMargin) { clipBitField     |= topBitMask;                      }
		if (vertex.position.z <  -wPlusMargin) { clipBitField     |= nearBitMask;                     }
		if (vertex.position.z >   wPlusMargin) { clipBitField     |= farBitMask;                      }

		if (vertex.position.x <= -wLessMargin) { equalityBitField |= (~clipBitField) & leftBitMask;   }
		if (vertex.position.x >=  wLessMargin) { equalityBitField |= (~clipBitField) & rightBitMask;  }
		if (vertex.position.y <= -wLessMargin) { equalityBitField |= (~clipBitField) & bottomBitMask; }
		if (vertex.position.y >=  wLessMargin) { equalityBitField |= (~clipBitField) & topBitMask;    }
		if (vertex.position.z <= -wLessMargin) { equalityBitField |= (~clipBitField) & nearBitMask;   }
		if (vertex.position.z >=  wLessMargin) { equalityBitField |= (~clipBitField) & farBitMask;    }
	}

	if (!(vertexClipBitFields[0] | vertexClipBitFields[1] | vertexClipBitFields[2]))
	{
		drawTriangle(vertices, texture, halfWidth, halfHeight, colorBuffer, depthBuffer);
	}
	else if ((vertexClipBitFields[0] | vertexEqualityBitFields[0]) &
	         (vertexClipBitFields[1] | vertexEqualityBitFields[1]) &
	         (vertexClipBitFields[2] | vertexEqualityBitFields[2]))
	{
		return;
	}
	else
	{
		constexpr EdgeInfo edges[3] = { { 0, 1, 2 }, { 1, 2, 0 }, { 2, 0, 1 } };

		for (const EdgeInfo& edge : edges)
		{
			const Vertex firstVertex    = vertices[edge.firstVertexIndex];
			const Vertex secondVertex   = vertices[edge.secondVertexIndex];
			const Vertex oppositeVertex = vertices[edge.oppositeVertexIndex];

			const unsigned char combinedField = (vertexClipBitFields[edge.firstVertexIndex] ^ vertexClipBitFields[edge.secondVertexIndex]) & ~(vertexEqualityBitFields[edge.firstVertexIndex] | vertexEqualityBitFields[edge.secondVertexIndex]);

			if (combinedField)
			{
				Vertex intersection;

				if      (combinedField & leftBitMask)   { intersection = lineFrustumIntersection(firstVertex, secondVertex, Axis::X, true ); } 
				else if (combinedField & rightBitMask)  { intersection = lineFrustumIntersection(firstVertex, secondVertex, Axis::X, false); }
				else if (combinedField & bottomBitMask) { intersection = lineFrustumIntersection(firstVertex, secondVertex, Axis::Y, true ); }
				else if (combinedField & topBitMask)    { intersection = lineFrustumIntersection(firstVertex, secondVertex, Axis::Y, false); }
				else if (combinedField & nearBitMask)   { intersection = lineFrustumIntersection(firstVertex, secondVertex, Axis::Z, true ); }
				else if (combinedField & farBitMask)    { intersection = lineFrustumIntersection(firstVertex, secondVertex, Axis::Z, false); }
				else
				{
					assert(false);
				}

				clipAndDrawTriangle({ firstVertex,  intersection,   oppositeVertex }, texture, halfWidth, halfHeight, colorBuffer, depthBuffer);
				clipAndDrawTriangle({ secondVertex, oppositeVertex, intersection   }, texture, halfWidth, halfHeight, colorBuffer, depthBuffer);

				break;
			}
		}
	}
}

float tr::Rasterizer::interpolate(const float weight0, const float value0, const float weight1, const float value1, const float weight2, const float value2)
{
	return weight0 * value0 + weight1 * value1 + weight2 * value2;
}

void tr::Rasterizer::pixelShift(std::array<Vertex, 3>& vertices)
{
	for (Vertex& vertex : vertices)
	{
		vertex.position.x -= 0.5f;
		vertex.position.y -= 0.5f;
	}
}

void tr::Rasterizer::perspectiveDivide(std::array<Vertex, 3>& vertices)
{
	for (Vertex& vertex : vertices)
	{
		vertex.position /= vertex.position.w;
	}
}

void tr::Rasterizer::viewportTransformation(std::array<Vertex, 3>& vertices, const float halfWidth, const float halfHeight)
{
	for (Vertex& vertex : vertices)
	{
		vertex.position.x = vertex.position.x * halfWidth + halfWidth;
		vertex.position.y = halfHeight - vertex.position.y * halfHeight;
	}
}

void tr::Rasterizer::sortVertices(std::array<Vertex, 3>& vertices)
{
	for (int iteration = 0; iteration < 2; ++iteration)
	{
		for (size_t vertexIndex = 0; vertexIndex < 2; ++vertexIndex)
		{
			if (vertices[vertexIndex].position.y > vertices[vertexIndex+1].position.y)
			{
				Vertex temp = vertices[vertexIndex];

				vertices[vertexIndex]   = vertices[vertexIndex+1];
				vertices[vertexIndex+1] = temp;
			}
		}
	}
}

void tr::Rasterizer::fillTriangle(const std::array<Vertex, 3>& vertices, const ColorBuffer& texture, ColorBuffer& colorBuffer, DepthBuffer& depthBuffer)
{
	const Vector2 top(   vertices[0].position.x, vertices[0].position.y);
	const Vector2 middle(vertices[1].position.x, vertices[1].position.y);
	const Vector2 bottom(vertices[2].position.x, vertices[2].position.y);

	const Vector2 topToMiddleVector    = (middle - top   ).normalize();
	const Vector2 topToBottomVector    = (bottom - top   ).normalize();
	const Vector2 middleToBottomVector = (bottom - middle).normalize();

	if (top.y != middle.y)
	{
		const size_t   firstY          = size_t(std::ceil(top.y));

		const float    topToFirstYDiff = float(firstY) - top.y;

		const Vector2& leftVector      = topToMiddleVector.x <= topToBottomVector.x ? topToMiddleVector : topToBottomVector;
		const Vector2& rightVector     = topToMiddleVector.x <= topToBottomVector.x ? topToBottomVector : topToMiddleVector;

		const float    startLeft       = top.x + leftVector.x  * topToFirstYDiff / leftVector.y;
		const float    startRight      = top.x + rightVector.x * topToFirstYDiff / rightVector.y;

		const size_t   targetY         = size_t(std::ceil(middle.y));

		fillTriangle(leftVector, rightVector, firstY, targetY, startLeft, startRight, colorBuffer);
	}

	if (middle.y != bottom.y)
	{
		const size_t   firstY             = size_t(std::ceil(middle.y));

		const float    middleToFirstYDiff = float(firstY) - middle.y;
		const float    topToFirstYDiff    = float(firstY) - top.y;

		const Vector2& leftVector         = topToMiddleVector.x <= topToBottomVector.x ? middleToBottomVector : topToBottomVector;
		const Vector2& rightVector        = topToMiddleVector.x <= topToBottomVector.x ? topToBottomVector    : middleToBottomVector;

		const float    ratioLeft          = (topToMiddleVector.x <= topToBottomVector.x ? middleToFirstYDiff : topToFirstYDiff)    / leftVector.y;
		const float    ratioRight         = (topToMiddleVector.x <= topToBottomVector.x ? topToFirstYDiff    : middleToFirstYDiff) / rightVector.y;

		const float    startLeft          = (topToMiddleVector.x <= topToBottomVector.x ? middle.x : top.x   ) + leftVector.x  * ratioLeft;
		const float    startRight         = (topToMiddleVector.x <= topToBottomVector.x ? top.x    : middle.x) + rightVector.x * ratioRight;

		const size_t   targetY            = size_t(std::ceil(bottom.y));

		fillTriangle(leftVector, rightVector, firstY, targetY, startLeft, startRight, colorBuffer);
	}
}

void tr::Rasterizer::fillTriangle(const Vector2& leftVector, const Vector2& rightVector, const size_t firstY, const size_t targetY, const float leftStart, const float rightStart, ColorBuffer& colorBuffer)
{
	float leftChange   = leftVector.x  / leftVector.y;
	float rightChange  = rightVector.x / rightVector.y;

	float currentLeft  = leftStart;
	float currentRight = rightStart;

	for (size_t currentY = firstY; currentY < targetY; ++currentY, currentLeft += leftChange, currentRight += rightChange)
	{
		const size_t firstX       = size_t(std::ceil(currentLeft));
		const size_t lastX        = size_t(std::ceil(currentRight));
	
		Color*       colorPointer = colorBuffer.getData() + (currentY * colorBuffer.getWidth() + firstX);
	
		for (size_t x = firstX; x < lastX; ++x, ++colorPointer)
		{
			*colorPointer = Color{ 255, 255, 255, 255};
		}
	}
}

template<typename T>
float tr::Rasterizer::orientPoint(const Vector4& lineStart, const Vector4& lineEnd, const T& point)
{
	return (lineEnd.x - lineStart.x) * (point.y - lineStart.y) - (lineEnd.y - lineStart.y) * (point.x - lineStart.x);
}
