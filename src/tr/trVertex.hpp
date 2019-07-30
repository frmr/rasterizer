#pragma once

#include "../matrix/Vectors.h"

namespace tr
{
	struct TransformedVertex
	{
		                   TransformedVertex();
		                   TransformedVertex(const Vector4& worldPosition, const Vector3& normal, const Vector2& textureCoord);
		                   TransformedVertex(const Vector4& worldPosition, const Vector4& position, const Vector3& normal, const Vector2& textureCoord);
		                   TransformedVertex(const Vector4& worldPosition, const Vector4& position, const Vector3& normal, const Vector2& textureCoord, const float inverseW);

		TransformedVertex& operator+=(const TransformedVertex& rhs);
		TransformedVertex& operator-=(const TransformedVertex& rhs);

		TransformedVertex  operator+(const TransformedVertex& rhs) const;
		TransformedVertex  operator-(const TransformedVertex& rhs) const;
		TransformedVertex  operator*(const float& rhs) const;
		TransformedVertex  operator/(const float& rhs) const;

		TransformedVertex& normalize();

		Vector4            worldPosition;
		Vector4            position;
		Vector3            normal;
		Vector2            textureCoord;
		float              inverseW;
	};
}