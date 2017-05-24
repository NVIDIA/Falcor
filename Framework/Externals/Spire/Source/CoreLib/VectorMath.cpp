#include "VectorMath.h"

namespace VectorMath
{
	const __m128 Matrix4_M128::VecOne = _mm_set_ps1(1.0f);
	void Matrix4::Rotation(Matrix4 & rs, const Vec3 & axis, float angle)
	{
		float c = cosf(angle);
		float s = sinf(angle);
		float t = 1.0f - c;

		Vec3 nAxis;
		Vec3::Normalize(nAxis, axis);
		float x = nAxis.x;
		float y = nAxis.y;
		float z = nAxis.z;

		rs.m[0][0] = 1 + t*(x*x-1);
		rs.m[1][0] = z*s+t*x*y;
		rs.m[2][0] = -y*s+t*x*z;
		rs.m[3][0] = 0.0f;

		rs.m[0][1] = -z*s+t*x*y;
		rs.m[1][1] = 1+t*(y*y-1);
		rs.m[2][1] = x*s+t*y*z;
		rs.m[3][1] = 0.0f;

		rs.m[0][2] = y*s+t*x*z;
		rs.m[1][2] = -x*s+t*y*z;
		rs.m[2][2] = 1+t*(z*z-1);
		rs.m[3][2] = 0.0f;

		rs.m[0][3] = 0.0f;
		rs.m[1][3] = 0.0f;
		rs.m[2][3] = 0.0f;
		rs.m[3][3] = 1.0f;
	}
	void Matrix4::Rotation(Matrix4 & rs, float yaw, float pitch, float roll)
	{
		Matrix4 mat;
		Matrix4::RotationY(rs, yaw);
		Matrix4::RotationX(mat, pitch);
		Matrix4::Multiply(rs, rs, mat);
		Matrix4::RotationZ(mat, roll);
		Matrix4::Multiply(rs, rs, mat);
	}

	void Matrix4::GetNormalMatrix(Matrix4 & mOut) const
	{
		float fDet = (mi._11 * (mi._22 * mi._33 - mi._23 * mi._32) -
			mi._12 * (mi._21 * mi._33 - mi._23 * mi._31) +
			mi._13 * (mi._21 * mi._32 - mi._22 * mi._31));
		float fDetInv = 1.0f / fDet;

		mOut.mi._11 = fDetInv * (mi._22 * mi._33 - mi._23 * mi._32);
		mOut.mi._21 = -fDetInv * (mi._12 * mi._33 - mi._13 * mi._32);
		mOut.mi._31 = fDetInv * (mi._12 * mi._23 - mi._13 * mi._22);

		mOut.mi._12 = -fDetInv * (mi._21 * mi._33 - mi._23 * mi._31);
		mOut.mi._22 = fDetInv * (mi._11 * mi._33 - mi._13 * mi._31);
		mOut.mi._32 = -fDetInv * (mi._11 * mi._23 - mi._13 * mi._21);

		mOut.mi._13 = fDetInv * (mi._21 * mi._32 - mi._22 * mi._31);
		mOut.mi._23 = -fDetInv * (mi._11 * mi._32 - mi._12 * mi._31);
		mOut.mi._33 = fDetInv * (mi._11 * mi._22 - mi._12 * mi._21);

		mOut.mi._14 = 0.0f;
		mOut.mi._24 = 0.0f;
		mOut.mi._34 = 0.0f;
		mOut.mi._41 = 0.0f;
		mOut.mi._42 = 0.0f;
		mOut.mi._43 = 0.0f;
		mOut.mi._44 = 1.0f;
	}
	
	float Matrix4::Inverse3D(Matrix4 & mOut_d) const
	{
		if(fabs(mi._44 - 1.0f) > 0.001f)
			return 0.0f;
		if(fabs(mi._14)>0.001f || fabs(mi._24)>0.001f || fabs(mi._34)>0.001f)
			return 0.0f;

		float fDet = (mi._11 * (mi._22 * mi._33 - mi._23 * mi._32) - 
		mi._12 * (mi._21 * mi._33 - mi._23 * mi._31) +
		mi._13 * (mi._21 * mi._32 - mi._22 * mi._31));
		float fDetInv = 1.0f / fDet;

		mOut_d.mi._11 = fDetInv * (mi._22 * mi._33 - mi._23 * mi._32);
		mOut_d.mi._12 = -fDetInv * (mi._12 * mi._33 - mi._13 * mi._32);
		mOut_d.mi._13 = fDetInv * (mi._12 * mi._23 - mi._13 * mi._22);
		mOut_d.mi._14 = 0.0f;

		mOut_d.mi._21 = -fDetInv * (mi._21 * mi._33 - mi._23 * mi._31);
		mOut_d.mi._22 = fDetInv * (mi._11 * mi._33 - mi._13 * mi._31);
		mOut_d.mi._23 = -fDetInv * (mi._11 * mi._23 - mi._13 * mi._21);
		mOut_d.mi._24 = 0.0f;

		mOut_d.mi._31 = fDetInv * (mi._21 * mi._32 - mi._22 * mi._31);
		mOut_d.mi._32 = -fDetInv * (mi._11 * mi._32 - mi._12 * mi._31);
		mOut_d.mi._33 = fDetInv * (mi._11 * mi._22 - mi._12 * mi._21);
		mOut_d.mi._34 = 0.0f;

		mOut_d.mi._41 = -(mi._41 * mOut_d.mi._11 + mi._42 * mOut_d.mi._21 + mi._43 * mOut_d.mi._31);
		mOut_d.mi._42 = -(mi._41 * mOut_d.mi._12 + mi._42 * mOut_d.mi._22 + mi._43 * mOut_d.mi._32);
		mOut_d.mi._43 = -(mi._41 * mOut_d.mi._13 + mi._42 * mOut_d.mi._23 + mi._43 * mOut_d.mi._33);
		mOut_d.mi._44 = 1.0f;

		return fDet;
	}
		
	float Matrix4::InverseFPU(Matrix4 &mOut_d) const
	{
		float succ = Inverse3D(mOut_d);
		if (succ != 0.0f)
			return succ;
		double Result[4][4];
		double tmp[12];
		double src[16];
		double det;
		for (int i = 0; i < 4; i++)
		{
			src[i+0] = m[i][0];
			src[i+4] = m[i][1];
			src[i+8] = m[i][2];
			src[i+12] = m[i][3];
		}
		tmp[0] = src[10] * src[15];
		tmp[1] = src[11] * src[14];
		tmp[2] = src[9] * src[15];
		tmp[3] = src[11] * src[13];
		tmp[4] = src[9] * src[14];
		tmp[5] = src[10] * src[13];
		tmp[6] = src[8] * src[15];
		tmp[7] = src[11] * src[12];
		tmp[8] = src[8] * src[14];
		tmp[9] = src[10] * src[12];
		tmp[10] = src[8] * src[13];
		tmp[11] = src[9] * src[12];
		Result[0][0] = tmp[0]*src[5] + tmp[3]*src[6] + tmp[4]*src[7];
		Result[0][0] -= tmp[1]*src[5] + tmp[2]*src[6] + tmp[5]*src[7];
		Result[0][1] = tmp[1]*src[4] + tmp[6]*src[6] + tmp[9]*src[7];
		Result[0][1] -= tmp[0]*src[4] + tmp[7]*src[6] + tmp[8]*src[7];
		Result[0][2] = tmp[2]*src[4] + tmp[7]*src[5] + tmp[10]*src[7];
		Result[0][2] -= tmp[3]*src[4] + tmp[6]*src[5] + tmp[11]*src[7];
		Result[0][3] = tmp[5]*src[4] + tmp[8]*src[5] + tmp[11]*src[6];
		Result[0][3] -= tmp[4]*src[4] + tmp[9]*src[5] + tmp[10]*src[6];
		Result[1][0] = tmp[1]*src[1] + tmp[2]*src[2] + tmp[5]*src[3];
		Result[1][0] -= tmp[0]*src[1] + tmp[3]*src[2] + tmp[4]*src[3];
		Result[1][1] = tmp[0]*src[0] + tmp[7]*src[2] + tmp[8]*src[3];
		Result[1][1] -= tmp[1]*src[0] + tmp[6]*src[2] + tmp[9]*src[3];
		Result[1][2] = tmp[3]*src[0] + tmp[6]*src[1] + tmp[11]*src[3];
		Result[1][2] -= tmp[2]*src[0] + tmp[7]*src[1] + tmp[10]*src[3];
		Result[1][3] = tmp[4]*src[0] + tmp[9]*src[1] + tmp[10]*src[2];
		Result[1][3] -= tmp[5]*src[0] + tmp[8]*src[1] + tmp[11]*src[2];
		tmp[0] = src[2]*src[7];
		tmp[1] = src[3]*src[6];
		tmp[2] = src[1]*src[7];
		tmp[3] = src[3]*src[5];
		tmp[4] = src[1]*src[6];
		tmp[5] = src[2]*src[5];
		tmp[6] = src[0]*src[7];
		tmp[7] = src[3]*src[4];
		tmp[8] = src[0]*src[6];
		tmp[9] = src[2]*src[4];
		tmp[10] = src[0]*src[5];
		tmp[11] = src[1]*src[4];
		Result[2][0] = tmp[0]*src[13] + tmp[3]*src[14] + tmp[4]*src[15];
		Result[2][0] -= tmp[1]*src[13] + tmp[2]*src[14] + tmp[5]*src[15];
		Result[2][1] = tmp[1]*src[12] + tmp[6]*src[14] + tmp[9]*src[15];
		Result[2][1] -= tmp[0]*src[12] + tmp[7]*src[14] + tmp[8]*src[15];
		Result[2][2] = tmp[2]*src[12] + tmp[7]*src[13] + tmp[10]*src[15];
		Result[2][2] -= tmp[3]*src[12] + tmp[6]*src[13] + tmp[11]*src[15];
		Result[2][3] = tmp[5]*src[12] + tmp[8]*src[13] + tmp[11]*src[14];
		Result[2][3] -= tmp[4]*src[12] + tmp[9]*src[13] + tmp[10]*src[14];
		Result[3][0] = tmp[2]*src[10] + tmp[5]*src[11] + tmp[1]*src[9];
		Result[3][0] -= tmp[4]*src[11] + tmp[0]*src[9] + tmp[3]*src[10];
		Result[3][1] = tmp[8]*src[11] + tmp[0]*src[8] + tmp[7]*src[10];
		Result[3][1] -= tmp[6]*src[10] + tmp[9]*src[11] + tmp[1]*src[8];
		Result[3][2] = tmp[6]*src[9] + tmp[11]*src[11] + tmp[3]*src[8];
		Result[3][2] -= tmp[10]*src[11] + tmp[2]*src[8] + tmp[7]*src[9];
		Result[3][3] = tmp[10]*src[10] + tmp[4]*src[8] + tmp[9]*src[9];
		Result[3][3] -= tmp[8]*src[9] + tmp[11]*src[10] + tmp[5]*src[8];
		det=src[0]*Result[0][0]+src[1]*Result[0][1]+src[2]*Result[0][2]+src[3]*Result[0][3];
		det = 1.0f / det;
		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				mOut_d.m[i][j] = (float)(Result[i][j] * det);
			}
		}
		return (float)det;
	}
	
	void Matrix4::LookAt(Matrix4 & rs, const Vec3 & pos, const Vec3 & center, const Vec3 & up)
	{
		Vec3 xAxis, yAxis, zAxis;
		Vec3::Subtract(zAxis, pos, center);
		Vec3::Normalize(zAxis, zAxis);
		Vec3::Cross(xAxis, up, zAxis);
		Vec3::Normalize(xAxis, xAxis);
		Vec3::Cross(yAxis, zAxis, xAxis);
		Vec3::Normalize(yAxis, yAxis);

		rs.m[0][0] = xAxis.x;
		rs.m[0][1] = yAxis.x;
		rs.m[0][2] = zAxis.x;
		rs.m[0][3] = 0.0f;

		rs.m[1][0] = xAxis.y;
		rs.m[1][1] = yAxis.y;
		rs.m[1][2] = zAxis.y;
		rs.m[1][3] = 0.0f;

		rs.m[2][0] = xAxis.z;
		rs.m[2][1] = yAxis.z;
		rs.m[2][2] = zAxis.z;
		rs.m[2][3] = 0.0f;

		rs.m[3][0] = -Vec3::Dot(xAxis, pos);
		rs.m[3][1] = -Vec3::Dot(yAxis, pos);
		rs.m[3][2] = -Vec3::Dot(zAxis, pos);
		rs.m[3][3] = 1.0f;
	}
	
	float Matrix4_M128::Inverse(Matrix4_M128 &mOut) const
	{
		__m128 Fac0;
		{
			__m128 Swp0a = _mm_shuffle_ps(C4, C3, _MM_SHUFFLE(3, 3, 3, 3));
			__m128 Swp0b = _mm_shuffle_ps(C4, C3, _MM_SHUFFLE(2, 2, 2, 2));

			__m128 Swp00 = _mm_shuffle_ps(C3, C2, _MM_SHUFFLE(2, 2, 2, 2));
			__m128 Swp01 = _mm_shuffle_ps(Swp0a, Swp0a, _MM_SHUFFLE(2, 0, 0, 0));
			__m128 Swp02 = _mm_shuffle_ps(Swp0b, Swp0b, _MM_SHUFFLE(2, 0, 0, 0));
			__m128 Swp03 = _mm_shuffle_ps(C3, C2, _MM_SHUFFLE(3, 3, 3, 3));

			__m128 Mul00 = _mm_mul_ps(Swp00, Swp01);
			__m128 Mul01 = _mm_mul_ps(Swp02, Swp03);
			Fac0 = _mm_sub_ps(Mul00, Mul01);
		}

		__m128 Fac1;
		{
			__m128 Swp0a = _mm_shuffle_ps(C4, C3, _MM_SHUFFLE(3, 3, 3, 3));
			__m128 Swp0b = _mm_shuffle_ps(C4, C3, _MM_SHUFFLE(1, 1, 1, 1));

			__m128 Swp00 = _mm_shuffle_ps(C3, C2, _MM_SHUFFLE(1, 1, 1, 1));
			__m128 Swp01 = _mm_shuffle_ps(Swp0a, Swp0a, _MM_SHUFFLE(2, 0, 0, 0));
			__m128 Swp02 = _mm_shuffle_ps(Swp0b, Swp0b, _MM_SHUFFLE(2, 0, 0, 0));
			__m128 Swp03 = _mm_shuffle_ps(C3, C2, _MM_SHUFFLE(3, 3, 3, 3));

			__m128 Mul00 = _mm_mul_ps(Swp00, Swp01);
			__m128 Mul01 = _mm_mul_ps(Swp02, Swp03);
			Fac1 = _mm_sub_ps(Mul00, Mul01);
		}

		__m128 Fac2;
		{
			__m128 Swp0a = _mm_shuffle_ps(C4, C3, _MM_SHUFFLE(2, 2, 2, 2));
			__m128 Swp0b = _mm_shuffle_ps(C4, C3, _MM_SHUFFLE(1, 1, 1, 1));

			__m128 Swp00 = _mm_shuffle_ps(C3, C2, _MM_SHUFFLE(1, 1, 1, 1));
			__m128 Swp01 = _mm_shuffle_ps(Swp0a, Swp0a, _MM_SHUFFLE(2, 0, 0, 0));
			__m128 Swp02 = _mm_shuffle_ps(Swp0b, Swp0b, _MM_SHUFFLE(2, 0, 0, 0));
			__m128 Swp03 = _mm_shuffle_ps(C3, C2, _MM_SHUFFLE(2, 2, 2, 2));

			__m128 Mul00 = _mm_mul_ps(Swp00, Swp01);
			__m128 Mul01 = _mm_mul_ps(Swp02, Swp03);
			Fac2 = _mm_sub_ps(Mul00, Mul01);
		}

		__m128 Fac3;
		{
			__m128 Swp0a = _mm_shuffle_ps(C4, C3, _MM_SHUFFLE(3, 3, 3, 3));
			__m128 Swp0b = _mm_shuffle_ps(C4, C3, _MM_SHUFFLE(0, 0, 0, 0));

			__m128 Swp00 = _mm_shuffle_ps(C3, C2, _MM_SHUFFLE(0, 0, 0, 0));
			__m128 Swp01 = _mm_shuffle_ps(Swp0a, Swp0a, _MM_SHUFFLE(2, 0, 0, 0));
			__m128 Swp02 = _mm_shuffle_ps(Swp0b, Swp0b, _MM_SHUFFLE(2, 0, 0, 0));
			__m128 Swp03 = _mm_shuffle_ps(C3, C2, _MM_SHUFFLE(3, 3, 3, 3));

			__m128 Mul00 = _mm_mul_ps(Swp00, Swp01);
			__m128 Mul01 = _mm_mul_ps(Swp02, Swp03);
			Fac3 = _mm_sub_ps(Mul00, Mul01);
		}

		__m128 Fac4;
		{
			__m128 Swp0a = _mm_shuffle_ps(C4, C3, _MM_SHUFFLE(2, 2, 2, 2));
			__m128 Swp0b = _mm_shuffle_ps(C4, C3, _MM_SHUFFLE(0, 0, 0, 0));

			__m128 Swp00 = _mm_shuffle_ps(C3, C2, _MM_SHUFFLE(0, 0, 0, 0));
			__m128 Swp01 = _mm_shuffle_ps(Swp0a, Swp0a, _MM_SHUFFLE(2, 0, 0, 0));
			__m128 Swp02 = _mm_shuffle_ps(Swp0b, Swp0b, _MM_SHUFFLE(2, 0, 0, 0));
			__m128 Swp03 = _mm_shuffle_ps(C3, C2, _MM_SHUFFLE(2, 2, 2, 2));

			__m128 Mul00 = _mm_mul_ps(Swp00, Swp01);
			__m128 Mul01 = _mm_mul_ps(Swp02, Swp03);
			Fac4 = _mm_sub_ps(Mul00, Mul01);
		}

		__m128 Fac5;
		{
			__m128 Swp0a = _mm_shuffle_ps(C4, C3, _MM_SHUFFLE(1, 1, 1, 1));
			__m128 Swp0b = _mm_shuffle_ps(C4, C3, _MM_SHUFFLE(0, 0, 0, 0));

			__m128 Swp00 = _mm_shuffle_ps(C3, C2, _MM_SHUFFLE(0, 0, 0, 0));
			__m128 Swp01 = _mm_shuffle_ps(Swp0a, Swp0a, _MM_SHUFFLE(2, 0, 0, 0));
			__m128 Swp02 = _mm_shuffle_ps(Swp0b, Swp0b, _MM_SHUFFLE(2, 0, 0, 0));
			__m128 Swp03 = _mm_shuffle_ps(C3, C2, _MM_SHUFFLE(1, 1, 1, 1));

			__m128 Mul00 = _mm_mul_ps(Swp00, Swp01);
			__m128 Mul01 = _mm_mul_ps(Swp02, Swp03);
			Fac5 = _mm_sub_ps(Mul00, Mul01);
		}

		__m128 SignA = _mm_set_ps( 1.0f,-1.0f, 1.0f,-1.0f);
		__m128 SignB = _mm_set_ps(-1.0f, 1.0f,-1.0f, 1.0f);

		__m128 Temp0 = _mm_shuffle_ps(C2, C1, _MM_SHUFFLE(0, 0, 0, 0));
		__m128 Vec0 = _mm_shuffle_ps(Temp0, Temp0, _MM_SHUFFLE(2, 2, 2, 0));

		__m128 Temp1 = _mm_shuffle_ps(C2, C1, _MM_SHUFFLE(1, 1, 1, 1));
		__m128 Vec1 = _mm_shuffle_ps(Temp1, Temp1, _MM_SHUFFLE(2, 2, 2, 0));

		__m128 Temp2 = _mm_shuffle_ps(C2, C1, _MM_SHUFFLE(2, 2, 2, 2));
		__m128 Vec2 = _mm_shuffle_ps(Temp2, Temp2, _MM_SHUFFLE(2, 2, 2, 0));

		__m128 Temp3 = _mm_shuffle_ps(C2, C1, _MM_SHUFFLE(3, 3, 3, 3));
		__m128 Vec3 = _mm_shuffle_ps(Temp3, Temp3, _MM_SHUFFLE(2, 2, 2, 0));

		__m128 Mul00 = _mm_mul_ps(Vec1, Fac0);
		__m128 Mul01 = _mm_mul_ps(Vec2, Fac1);
		__m128 Mul02 = _mm_mul_ps(Vec3, Fac2);
		__m128 Sub00 = _mm_sub_ps(Mul00, Mul01);
		__m128 Add00 = _mm_add_ps(Sub00, Mul02);
		__m128 Inv0 = _mm_mul_ps(SignB, Add00);

		__m128 Mul03 = _mm_mul_ps(Vec0, Fac0);
		__m128 Mul04 = _mm_mul_ps(Vec2, Fac3);
		__m128 Mul05 = _mm_mul_ps(Vec3, Fac4);
		__m128 Sub01 = _mm_sub_ps(Mul03, Mul04);
		__m128 Add01 = _mm_add_ps(Sub01, Mul05);
		__m128 Inv1 = _mm_mul_ps(SignA, Add01);

		__m128 Mul06 = _mm_mul_ps(Vec0, Fac1);
		__m128 Mul07 = _mm_mul_ps(Vec1, Fac3);
		__m128 Mul08 = _mm_mul_ps(Vec3, Fac5);
		__m128 Sub02 = _mm_sub_ps(Mul06, Mul07);
		__m128 Add02 = _mm_add_ps(Sub02, Mul08);
		__m128 Inv2 = _mm_mul_ps(SignB, Add02);

		__m128 Mul09 = _mm_mul_ps(Vec0, Fac2);
		__m128 Mul10 = _mm_mul_ps(Vec1, Fac4);
		__m128 Mul11 = _mm_mul_ps(Vec2, Fac5);
		__m128 Sub03 = _mm_sub_ps(Mul09, Mul10);
		__m128 Add03 = _mm_add_ps(Sub03, Mul11);
		__m128 Inv3 = _mm_mul_ps(SignA, Add03);

		__m128 Row0 = _mm_shuffle_ps(Inv0, Inv1, _MM_SHUFFLE(0, 0, 0, 0));
		__m128 Row1 = _mm_shuffle_ps(Inv2, Inv3, _MM_SHUFFLE(0, 0, 0, 0));
		__m128 Row2 = _mm_shuffle_ps(Row0, Row1, _MM_SHUFFLE(2, 0, 2, 0));

		// Det0 = dot(C1, Row2)
		__m128 mul0 = _mm_mul_ps(C1, Row2);
		__m128 swp0 = _mm_shuffle_ps(mul0, mul0, _MM_SHUFFLE(2, 3, 0, 1));
		__m128 add0 = _mm_add_ps(mul0, swp0);
		__m128 swp1 = _mm_shuffle_ps(add0, add0, _MM_SHUFFLE(0, 1, 2, 3));
		__m128 Det0 = _mm_add_ps(add0, swp1);

		__m128 Rcp0 = _mm_div_ps(VecOne, Det0);

		mOut.C1 = _mm_mul_ps(Inv0, Rcp0);
		mOut.C2 = _mm_mul_ps(Inv1, Rcp0);
		mOut.C3 = _mm_mul_ps(Inv2, Rcp0);
		mOut.C4 = _mm_mul_ps(Inv3, Rcp0);

		float retVal;
		_mm_store_ss(&retVal, Det0);
		return retVal;
	}

}
