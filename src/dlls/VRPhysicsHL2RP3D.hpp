
// Only to be included by VRPhysicsHelper.cpp

// HL to RP3D conversion helper functions
namespace
{
	inline Vector3 HLVecToRP3DVec(const Vector& hlVec)
	{
		return Vector3{ hlVec.x * HL_TO_RP3D, hlVec.y * HL_TO_RP3D, hlVec.z * HL_TO_RP3D };
	}

	inline Vector RP3DVecToHLVec(const Vector3& rp3dVec)
	{
		return Vector{ float(rp3dVec.x * RP3D_TO_HL), float(rp3dVec.y * RP3D_TO_HL), float(rp3dVec.z * RP3D_TO_HL) };
	}

	inline rp3d::Quaternion HLAnglesToRP3DQuaternion(const Vector& angles)
	{
		// up / down
#define PITCH 0
// left / right
#define YAW 1
// fall over
#define ROLL 2

		rp3d::Quaternion rollQuaternion;
		rollQuaternion.setAllValues(sin(double(angles[ROLL]) * M_PI / 360.), 0., 0., cos(double(angles[ROLL]) * M_PI / 360.));

		rp3d::Quaternion pitchQuaternion;
		pitchQuaternion.setAllValues(0., sin(double(angles[PITCH]) * M_PI / 360.), 0., cos(double(angles[PITCH]) * M_PI / 360.));

		rp3d::Quaternion yawQuaternion;
		yawQuaternion.setAllValues(0., 0., sin(double(angles[YAW]) * M_PI / 360.), cos(double(angles[YAW]) * M_PI / 360.));

		return yawQuaternion * pitchQuaternion * rollQuaternion;
	}

	inline Vector RP3DTransformToHLAngles(const rp3d::Matrix3x3& matrix)
	{
		Vector forward{ float(matrix.getRow(0).x), float(matrix.getRow(0).y), float(matrix.getRow(0).z) };
		Vector right{ float(matrix.getRow(1).x), float(matrix.getRow(1).y), float(matrix.getRow(1).z) };
		Vector up{ float(matrix.getRow(2).x), float(matrix.getRow(2).y), float(matrix.getRow(2).z) };

		Vector angles;
		UTIL_GetAnglesFromVectors(forward, right, up, angles);

		return Vector{ angles.x, angles.y, angles.z };
	}
}


