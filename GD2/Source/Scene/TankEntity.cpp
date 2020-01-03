/*******************************************
	TankEntity.cpp

	Tank entity template and entity classes
********************************************/

// Additional technical notes for the assignment:
// - Each tank has a team number (0 or 1), HP and other instance data - see the end of TankEntity.h
//   You will need to add other instance data suitable for the assignment requirements
// - A function GetTankUID is defined in TankAssignment.cpp and made available here, which returns
//   the UID of the tank on a given team. This can be used to get the enemy tank UID
// - Tanks have three parts: the root, the body and the turret. Each part has its own matrix, which
//   can be accessed with the Matrix function - root: Matrix(), body: Matrix(1), turret: Matrix(2)
//   However, the body and turret matrix are relative to the root's matrix - so to get the actual 
//   world matrix of the body, for example, we must multiply: Matrix(1) * Matrix()
// - Vector facing work similar to the car tag lab will be needed for the turret->enemy facing 
//   requirements for the Patrol and Aim states
// - The CMatrix4x4 function DecomposeAffineEuler allows you to extract the x,y & z rotations
//   of a matrix. This can be used on the *relative* turret matrix to help in rotating it to face
//   forwards in Evade state
// - The CShellEntity class is simply an outline. To support shell firing, you will need to add
//   member data to it and rewrite its constructor & update function. You will also need to update 
//   the CreateShell function in EntityManager.cpp to pass any additional constructor data required
// - Destroy an entity by returning false from its Update function - the entity manager wil perform
//   the destruction. Don't try to call DestroyEntity from within the Update function.
// - As entities can be destroyed, you must check that entity UIDs refer to existant entities, before
//   using their entity pointers. The return value from EntityManager.GetEntity will be NULL if the
//   entity no longer exists. Use this to avoid trying to target a tank that no longer exists etc.

#include "TankEntity.h"
#include "EntityManager.h"
#include "Messenger.h"

namespace gen
{

	// Reference to entity manager from TankAssignment.cpp, allows look up of entities by name, UID etc.
	// Can then access other entity's data. See the CEntityManager.h file for functions. Example:
	//    CVector3 targetPos = EntityManager.GetEntity( targetUID )->GetMatrix().Position();
	extern CEntityManager EntityManager;

	// Messenger class for sending messages to and between entities
	extern CMessenger Messenger;

	// Helper function made available from TankAssignment.cpp - gets UID of tank A (team 0) or B (team 1).
	// Will be needed to implement the required tank behaviour in the Update function below
	extern TEntityUID GetTankUID(int team);

	bool SphereToSphere(CVector3 A, CVector3 B)
	{
		if (A.x > B.x - 20 && A.x < B.x + 20 &&
			A.y > B.y - 20 && A.y < B.y + 20 &&
			A.z > B.z - 20 && A.z < B.z + 20)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	void CTankEntity::UpdateTankData(int Index)
	{
		m_Target = GetTankUID(Index);
		TankTarget = EntityManager.GetEntity(m_Target);
		TargetTank = static_cast<CTankEntity*>(TankTarget);
		if (TargetTank != NULL)
		{
			TurretWorldMatrix = Matrix(2) * Matrix();
			TankFacingVector = TurretWorldMatrix.ZAxis();
			DistanceVector = TankTarget->Matrix().Position() - Matrix().Position();
			DistanceVector.Normalise();
		}
	}
	float AngleMath(CMatrix4x4 TurretWorldMatrix, CVector3 TankFacingVector, CVector3 DistanceVector)
	{
		float MagnitudeA = (TankFacingVector.x * TankFacingVector.x) + (TankFacingVector.y * TankFacingVector.y) + (TankFacingVector.z * TankFacingVector.z);
		float MagnitudeB = (DistanceVector.x * DistanceVector.x) + (DistanceVector.y * DistanceVector.y) + (DistanceVector.z * DistanceVector.z);
		float DotProduct = Dot(TankFacingVector, DistanceVector);
		float Theta = DotProduct / sqrt(MagnitudeA) * sqrt(MagnitudeB);
		float Angle = ACos(Theta) * 180.0f / 3.14159265359f;
		return Angle;
	}
	CVector3 RandomPosChecker(CVector3 MatrixPos, CVector3 RanPos)
	{
		if (SphereToSphere(MatrixPos, RanPos))
		{
			RanPos = CVector3(Random(MatrixPos.x - 40, MatrixPos.x + 40), 0.5, Random(MatrixPos.x - 40, MatrixPos.x + 40));
			RandomPosChecker(MatrixPos, RanPos);
		}
		else
		{
			return RanPos;
		}
	}

	/*-----------------------------------------------------------------------------------------
	-------------------------------------------------------------------------------------------
		Tank Entity Class
	-------------------------------------------------------------------------------------------
	-----------------------------------------------------------------------------------------*/

	// Tank constructor intialises tank-specific data and passes its parameters to the base
	// class constructor
	CTankEntity::CTankEntity
	(
		CTankTemplate* tankTemplate,
		TEntityUID      UID,
		TUInt32         team,
		const string& name /*=""*/,
		const CVector3& position /*= CVector3::kOrigin*/,
		const CVector3& rotation /*= CVector3( 0.0f, 0.0f, 0.0f )*/,
		const CVector3& scale /*= CVector3( 1.0f, 1.0f, 1.0f )*/
	) : CEntity(tankTemplate, UID, name, position, rotation, scale)
	{
		m_TankTemplate = tankTemplate;

		// Tanks are on teams so they know who the enemy is
		m_Team = team;

		// Initialise other tank data and state
		m_Speed = 0.0f;
		m_HP = m_TankTemplate->GetMaxHP();
		m_State = Inactive;
		m_Timer = 0.0f;
	}


	// Update the tank - controls its behaviour. The shell code just performs some test behaviour, it
	// is to be rewritten as one of the assignment requirements
	// Return false if the entity is to be destroyed
	bool CTankEntity::Update(TFloat32 updateTime)
	{
		// Fetch any messages
		SMessage msg;
		while (Messenger.FetchMessage(GetUID(), &msg))
		{
			// Set state variables based on received messages
			switch (msg.type)
			{
			case Msg_Start:
				m_State = Patrol;
				break;
			case Msg_Hit:
				this->m_HP -= 20;
				break;
			case Msg_Stop:
				m_State = Inactive;
				break;
			}
		}

		// Tank behaviour
		// Only move if in Go state
		//if (m_State == Go)
		//{
		//	// Cycle speed up and down using a sine wave - just demonstration behaviour
		//	//**** Variations on this sine wave code does not count as patrolling - for the
		//	//**** assignment the tank must move naturally between two specific points
		//	m_Speed = 10.0f * Sin( m_Timer * 4.0f );
		//	m_Timer += updateTime;
		//}
		//else
		//{
		//	m_Speed = 0;
		//}
		if (m_State == Inactive)
		{

		}
		if (m_State == Evade)
		{
			RandomPos = RandomPosChecker(Matrix().Position(), RandomPos);
			Fired = false;
			m_Speed = 10.0f;
			CVector3 Rotation;
			Matrix(2).DecomposeAffineEuler(NULL, &Rotation, NULL);
			if (Rotation.y < 0)
			{
				Matrix(2).RotateLocalY(1 * updateTime);
			}
			if (Rotation.y > 0)
			{
				Matrix(2).RotateLocalY(-1 * updateTime);
			}
			CMatrix4x4 BodyMatrix = Matrix(1) * Matrix(0);
			CVector3 Facing = -BodyMatrix.ZAxis();
			targetPos = this->RandomPos;
			CVector3 DistanceVect = Matrix().Position() - targetPos;
			DistanceVect.Normalise();
			float Angle = AngleMath(BodyMatrix, Facing, DistanceVect);
			if (Angle < 0.5f)
			{
				Matrix().FaceTarget(RandomPos);
				Matrix().MoveLocalZ(m_Speed * updateTime);
			}
			else
			{
				Matrix().MoveLocalZ(m_Speed / 2 * updateTime);
				Matrix().RotateLocalY(1.0f * updateTime);
			}
			if (SphereToSphere(Matrix().GetPosition(), this->RandomPos))
			{
				m_State = Patrol;
				this->RandomPos = CVector3(Random(-40, 40), 0.5, Random(-40, 40));
			}

		}
		if (m_State == Aim)
		{
			for (int i = 0; i < 2; i++)
			{
				if (Fired == false)
				{
					m_Target = GetTankUID(i);
					if (m_Target != this->GetUID())
					{
						UpdateTankData(i);
						if (TargetTank != NULL)
						{
							Angle = AngleMath(this->TurretWorldMatrix, this->TankFacingVector, this->DistanceVector);
							float DotProduct = Dot(DistanceVector, this->TankFacingVector);
							if (this->Timer >= 0.0f)
							{
								Timer -= updateTime;
								if (Angle > 0.5f)
								{
									Matrix(2).RotateLocalY(2 * updateTime);
								}
								if (Angle < 0.5f)
								{
									Matrix(2).RotateLocalY(-2 * updateTime);
								}
								m_Speed = 0.0f;

							}
							if (this->Timer <= 0)
							{
								CVector3 TurretRot;
								this->TurretWorldMatrix.DecomposeAffineEuler(NULL, &TurretRot, NULL);
								CMatrix4x4 NewMatrix = Matrix(2) * Matrix();
								Timer = 1.0f;
								EntityManager.CreateTemplate("Projectile", "Shell Type 1", "Bullet.x");
								EntityManager.CreateShell("Shell Type 1", GetName(), this->TurretWorldMatrix.Position(), TurretRot);
								m_State = Evade;
								Fired = true;
							}
						}
						else
						{
							m_State = Patrol;
						}

					}
				}
			}
		}
		if (m_State == Patrol)
		{
			float TankUIDT1 = GetTankUID(0);
			float TankUIDT2 = GetTankUID(1);
			CMatrix4x4 BodyMatrix = Matrix(1) * Matrix();
			CVector3 Facing = -BodyMatrix.ZAxis();
			if (TankUIDT1 == this->GetUID())
			{
				if (SphereToSphere(Matrix().GetPosition(), PatrolPosT1[0]))
				{
					this->AtTarget = true;
				}
				if (this->AtTarget == false)
				{
					targetPos = PatrolPosT1[0];
					CVector3 DistanceVect = Matrix().Position() - PatrolPosT1[0];
					DistanceVect.Normalise();
					float Angle = AngleMath(BodyMatrix, Facing, DistanceVect);
					if (Angle < 0.5f)
					{
						Matrix().FaceTarget(targetPos);
						Matrix().MoveLocalZ(m_Speed * updateTime);
					}
					else
					{
						Matrix().MoveLocalZ(m_Speed / 5 * updateTime);
						Matrix().RotateLocalY(1.0f * updateTime);
					}
				}
				if (SphereToSphere(Matrix().GetPosition(), PatrolPosT1[1]))
				{
					this->AtTarget = false;
				}
				if (this->AtTarget == true)
				{
					targetPos = PatrolPosT1[1];
					CVector3 DistanceVect = BodyMatrix.Position() - PatrolPosT1[1];
					DistanceVect.Normalise();
					float Angle = AngleMath(BodyMatrix, Facing, DistanceVect);
					if (Angle < 0.5f)
					{
						Matrix().FaceTarget(targetPos);
						Matrix().MoveLocalZ(m_Speed * updateTime);
					}
					else
					{
						Matrix().MoveLocalZ(m_Speed / 5 * updateTime);
						Matrix().RotateLocalY(1.0f * updateTime);
					}
				}
			}
			if (TankUIDT2 == this->GetUID())
			{
				if (AtTarget == false)
				{
					targetPos = PatrolPosT2[0];
					CVector3 DistanceVect = Matrix().Position() - PatrolPosT2[0];
					DistanceVect.Normalise();
					float Angle = AngleMath(BodyMatrix, Facing, DistanceVect);
					if (Angle < 0.5f)
					{
						Matrix().FaceTarget(targetPos);
						Matrix().MoveLocalZ(m_Speed * updateTime);
					}
					else
					{
						Matrix().MoveLocalZ(m_Speed / 5 * updateTime);
						Matrix().RotateLocalY(1.0f * updateTime);
					}
				}
				if (SphereToSphere(Matrix().GetPosition(), PatrolPosT2[0]))
				{
					AtTarget = true;
				}
				if (AtTarget == true)
				{
					targetPos = PatrolPosT2[1];
					CVector3 DistanceVect = Matrix().Position() - PatrolPosT2[1];
					DistanceVect.Normalise();
					float Angle = AngleMath(BodyMatrix, Facing, DistanceVect);
					if (Angle < 0.5f)
					{
						Matrix().FaceTarget(targetPos);
						Matrix().MoveLocalZ(m_Speed * updateTime);
					}
					else
					{
						Matrix().MoveLocalZ(m_Speed / 5 * updateTime);
						Matrix().RotateLocalY(1.0f * updateTime);
					}
				}
				if (SphereToSphere(Matrix().GetPosition(), PatrolPosT2[1]))
				{
					AtTarget = false;
				}
			}
			m_Speed = 10.0f;
			m_TankTemplate->GetMaxSpeed();
			m_Timer += m_TankTemplate->GetAcceleration();
			if (m_Timer > m_TankTemplate->GetMaxSpeed())
			{
				m_Timer = m_TankTemplate->GetMaxSpeed();
			}

			for (int i = 0; i < 2; i++)
			{
				UpdateTankData(i);
				if (m_Target != this->GetUID())
				{
					Angle = AngleMath(this->TurretWorldMatrix, this->TankFacingVector, this->DistanceVector);
					if (Angle < 15.0f)
					{
						m_State = Aim;
					}
					else
					{
						Matrix(2).RotateLocalY(1 * updateTime);
					}
				}

			}
		}

		// Perform movement...
		// Move along local Z axis scaled by update time
		//Matrix().MoveLocalZ(m_Speed* updateTime);
		if (this->m_HP <= 0)
		{
			return false;
		}
		return true; // Don't destroy the entity
	}


} // namespa
