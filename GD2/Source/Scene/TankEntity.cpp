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

	/*Will determind wether the tank has line of sight*/
	bool LineOfSight(CVector3 TurretFacing, CMatrix4x4 TurretMatrix, CEntity* TankTarget)
	{
		

		EntityManager.BeginEnumEntities("", "", "Scenery");
		CEntity* Building = EntityManager.EnumEntity();
		while (Building != 0)
		{
			if (Building->GetName() == "Building")
			{
				/* Gets all the corners of the building*/
				CVector3 BuildingPoints[4];
				CVector3 buildingpointTR = Building->Position();
				buildingpointTR.x = Building->Position().x + 8.0f;
				buildingpointTR.z = Building->Position().z + 8.0f;
				BuildingPoints[0] = buildingpointTR;
		
				CVector3 buildingpointBR = Building->Position();
				buildingpointBR.x = Building->Position().x + 8.0f;
				buildingpointBR.z = Building->Position().z - 8.0f;
				BuildingPoints[1] = buildingpointBR;
		
				CVector3 buildingpointTL = Building->Position();
				buildingpointTL.x = Building->Position().x - 8.0f;
				buildingpointTL.z = Building->Position().z + 8.0f;
				BuildingPoints[2] = buildingpointTL;
		
				CVector3 buildingpointBL = Building->Position();
				buildingpointBL.x = Building->Position().x - 8.0f;
				buildingpointBL.z = Building->Position().z - 8.0f;
				BuildingPoints[3] = buildingpointBL;
		
				/* Will run through all the building points */
				for (int i = 0; i < 4; i++)
				{
					/* Gets the vector between tanks */
					CVector3 TankToTank = TankTarget->Position() - TurretMatrix.Position();
					/* Normalises the vector */
					CVector3 NormTankToTank = Normalise(TankToTank);
					CVector3 DistVector = BuildingPoints[i] - TurretMatrix.Position();
					/* Gets the length of the distance vector */
					float Dist = sqrt(LengthSquared(DistVector));
					/*Works out how far away we are*/
					CVector3 DistanceAway = TurretMatrix.Position() + Dist * NormTankToTank;
					/* Point to box to determine line of sight */
					if (DistanceAway.x > Building->Position().x - 8 && DistanceAway.x < Building->Position().x + 8)
					{
						if(DistanceAway.z > Building->Position().z - 6 && DistanceAway.z < Building->Position().z + 6)
						{
						return true;
						}
					}
				}
			}
			Building = EntityManager.EnumEntity();
		}
		EntityManager.EndEnumEntities();
		return false;
	}

	/*Sphere to sphere collision*/
	bool SphereToSphere(CVector3 A, CVector3 B)
	{
		if (A.x > B.x - 5 && A.x < B.x + 5 &&
			A.y > B.y - 5 && A.y < B.y + 5 &&
			A.z > B.z - 5 && A.z < B.z + 5)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	/*This will update all the tank targets so the list is never outdated*/
	void CTankEntity::UpdateTankTargets()
	{
		m_Target.clear();
		EntityManager.BeginEnumEntities("", "", "Tank");
		CEntity* entity = EntityManager.EnumEntity();
		int counter = 0;
		/* Will get all the enemies of the current team and will put them into the target list */
		while (entity != 0)
		{
			CTankEntity* TargetTank = static_cast<CTankEntity*>(entity);
			if (TargetTank != this && TargetTank != NULL && TargetTank->m_Team != this->m_Team)
			{

				m_Target.push_back(TargetTank->GetUID());
				++counter;
			}
			entity = EntityManager.EnumEntity();
		}
		EntityManager.EndEnumEntities();
	}

	/* This will update all the tank data that the tanks need */
	void CTankEntity::UpdateTankData(int index)
	{
		TankTarget = EntityManager.GetEntity(index);
		TargetTank = static_cast<CTankEntity*>(TankTarget);
		if (TargetTank != NULL)
		{
			TurretWorldMatrix = Matrix(2) * Matrix();
			TankFacingVector = TurretWorldMatrix.ZAxis();
			DistanceVector = TankTarget->Matrix().Position() - Matrix().Position();
			DistanceVector.Normalise();
		}
	}

	/*Used to find the angles between tanks*/
	float AngleMath(CMatrix4x4 TurretWorldMatrix, CVector3 TankFacingVector, CVector3 DistanceVector)
	{
		/*Gets the magnitudes of both vectors */
		float MagnitudeA = (TankFacingVector.x * TankFacingVector.x) + (TankFacingVector.y * TankFacingVector.y) + (TankFacingVector.z * TankFacingVector.z);
		float MagnitudeB = (DistanceVector.x * DistanceVector.x) + (DistanceVector.y * DistanceVector.y) + (DistanceVector.z * DistanceVector.z);
		/* Gets the dot product of the two vectors */
		float DotProduct = Dot(DistanceVector, TankFacingVector);
		/* Combine the two magnitudes */
		float Magnitude = sqrt(MagnitudeA) * sqrt(MagnitudeB);
		/* Get theta by diving the product by magnitude */
		float Theta = DotProduct / Magnitude;
		/* This will cap the theta so the acos doesn't break */
		if (Theta > 1)
		{
			Theta = 1.0f;
		}
		float ACOS = ACos(Theta);
		/* This will get the angle by transforming the Acos in randians */
		float Angle = ACOS * 180.0f / 3.14;
		return Angle;
	}
	/* This is used to check the random poses and to make sure the tanks aren't already on the randompos */
	CVector3 RandomPosChecker(CVector3 MatrixPos, CVector3 RanPos)
	{
		if (MatrixPos.x > RanPos.x - 5 && MatrixPos.x < RanPos.x + 5 &&
			MatrixPos.z > RanPos.z - 5 && MatrixPos.z < RanPos.z + 5)
		{
			RanPos = CVector3(Random(MatrixPos.x - 20, MatrixPos.x + 20), 0.5, Random(MatrixPos.z - 20, MatrixPos.z + 20));
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
		vector<CVector3> patrolPoints,
		const CVector3& position /*= CVector3::kOrigin*/,
		const CVector3& rotation /*= CVector3( 0.0f, 0.0f, 0.0f )*/,
		const CVector3& scale /*= CVector3( 1.0f, 1.0f, 1.0f )*/
	) : CEntity(tankTemplate, UID, name, position, rotation, scale)
	{
		m_TankTemplate = tankTemplate;

		// Tanks are on teams so they know who the enemy is
		m_Team = team;
		PatrolList = patrolPoints;
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
				UpdateTankTargets();
				targetPos = PatrolList.at(PatrolPointer);
				m_State = Patrol;
				break;
			case Msg_Hit:
				this->m_HP -= msg.damage;
				break;
			case Msg_Ammo:
				if (ShootsFired >= 5 && m_State != Dead)
				{
					m_State = Ammo;
				}
				break;
			case Msg_Health:
				if (m_HP <= 50 && m_State != Dead)
				{
					m_State = Health;
				}
				break;
			case Msg_Help:
				UpdateTankTargets();
				for (int i = 0; i < m_Target.size(); i++)
				{
					if (msg.from == m_Target.at(i))
					{
						CEntity* Entity = EntityManager.GetEntity(msg.from);
						CTankEntity* TankEntity = static_cast<CTankEntity*>(Entity);
						if (TankEntity->m_State != Dead)
						{
							SavedEnemyIndex = i;
						}
					}
				}
				m_State = Aim;
				break;
			case Msg_Stop:
				m_State = Inactive;
				break;
			}
		}

		/* Tanks start in this state */
		if (m_State == Inactive)
		{
			
		}
		/* This state is triggered when the the tanks have fired or can't shoot there target */
		if (m_State == Evade)
		{
			/*Check the random pos*/
			RandomPos = RandomPosChecker(Matrix().Position(), RandomPos);
			/* This will get the rotation of the turret */
			CVector3 Rotation;
			Matrix(2).DecomposeAffineEuler(NULL, &Rotation, NULL);
			/* Make the turns rotate to the front of the turret */
			if (Rotation.y < 0)
			{
				Matrix(2).RotateLocalY(m_TankTemplate->GetTurretTurnSpeed() * updateTime);
			}
			if (Rotation.y > 0)
			{
				Matrix(2).RotateLocalY(-m_TankTemplate->GetTurretTurnSpeed() * updateTime);
			}
			/*Gets the body matrix*/
			CMatrix4x4 BodyMatrix = Matrix(0) * Matrix(1);
			CVector3 Facing = -BodyMatrix.ZAxis();
			targetPos = this->RandomPos;
			CVector3 DistanceVect = Matrix().Position() - targetPos;
			DistanceVect.Normalise();
			/* Makes the speed faster if not already at the max*/
			if (m_Speed < m_TankTemplate->GetMaxSpeed())
			{
				m_Speed += m_TankTemplate->GetAcceleration();
			}
			/*Finds the angle*/
			float Angle = AngleMath(BodyMatrix, Facing, DistanceVect);
			/*Gets the dot product to determine weather it left or right*/
			float DotProduct = Dot(DistanceVect, BodyMatrix.XAxis());
			/* if the angle is dead ahead then more at normal speed*/
			if (Angle < 3.0f)
			{
				Matrix().FaceTarget(RandomPos);
				Matrix().MoveLocalZ(m_Speed * updateTime);
			}
			else
			{
				/* If the angle is not dead ahead this will determine which way the tank needs to turn */
				if (DotProduct > 0.0f)
				{
					Matrix().MoveLocalZ(m_TankTemplate->GetTurnSpeed() * updateTime);
					Matrix().RotateLocalY((-m_TankTemplate->GetTurnSpeed() / 2) * updateTime);
				}
				if (DotProduct < -0.0f)
				{
					Matrix().MoveLocalZ(m_TankTemplate->GetTurnSpeed() * updateTime);
					Matrix().RotateLocalY((m_TankTemplate->GetTurnSpeed() / 2) * updateTime);
				}
			}
			/* Check to see if the tank is already at the random pos */
			if (SphereToSphere(Matrix().GetPosition(), this->RandomPos))
			{
				m_State = Patrol;
				Fired = false;
				this->RandomPos = CVector3(Random(Matrix().Position().x - 20, Matrix().Position().x + 20), 0.5, Random(Matrix().Position().z - 20, Matrix().Position().z + 20));
			}
			/* If the tank is hit it will send this message out to all the other tanks on its team */
			if (msg.type == Msg_Hit)
			{
				EntityManager.BeginEnumEntities("", "", "Tank");
				CEntity* entity = EntityManager.EnumEntity();
				while (entity != 0)
				{
					CTankEntity* TankEntity = static_cast<CTankEntity*>(entity);
					if (TankEntity->GetTeam() == this->GetTeam())
					{
						/* Will only send to tanks that are not in there states */
						if (TankEntity != this && TankEntity->GetShootsFired() != 10 && TankEntity->m_State != TankEntity->Dead &&
							TankEntity->m_State != TankEntity->Aim && TankEntity->m_State != TankEntity->Ammo && TankEntity->m_State != TankEntity->Health
							)
						{
							TEntityUID UID = TankEntity->GetUID();
							msg.type = Msg_Help;
							//msg.from = SystemUID;
							Messenger.SendMessage(UID, msg);
						}
					}
					entity = EntityManager.EnumEntity();
				}
				EntityManager.EndEnumEntities();
			}

		}
		/* This state is used by the tanks to get a more accureate shot on the enemy tank */
		if (m_State == Aim)
		{
			/*Checks to see if the tanks ammo isn't empty*/
			if (this->ShootsFired != 10)
			{
				/* Check to see if the saved index isn't null */
				if (SavedEnemyIndex < m_Target.size())
				{
					/* Check to see if a shot has been fired */
					if (Fired == false)
					{
						CEntity* Tank = EntityManager.GetEntity(m_Target.at(SavedEnemyIndex));
						CTankEntity* TankEntity = static_cast<CTankEntity*>(Tank);
						/*Check to see if the target is dead*/
						if (m_Target[SavedEnemyIndex] != NULL && TankEntity->m_State != TankEntity->Dead)
						{
							UpdateTankData(m_Target[SavedEnemyIndex]);
						}
						/*Check to see if they have line of sight*/
						if (!LineOfSight(this->TankFacingVector, this->TurretWorldMatrix, TankTarget))
						{
							if (m_Target[SavedEnemyIndex] != NULL && TankEntity->m_State != TankEntity->Dead)
							{
								/*Gets the angle*/
								Angle = AngleMath(this->TurretWorldMatrix, this->TankFacingVector, this->DistanceVector);
								float DotProduct = Dot(DistanceVector, this->TurretWorldMatrix.XAxis());
								if (this->Timer >= 0.0f)
								{
									Timer -= updateTime;
									/*If the angle is in the correct rotation*/
									if (Angle < 2.0f)
									{
										TurretWorldMatrix.FaceTarget(TankTarget->Position());
									}
									/* Else it will turn the fasters way to reach the target */
									else if (DotProduct > 0.0f)
									{
										Matrix(2).RotateLocalY((m_TankTemplate->GetTurretTurnSpeed() * 2) * updateTime);
									}
									else if (DotProduct < -0.0f)
									{
										Matrix(2).RotateLocalY((-m_TankTemplate->GetTurretTurnSpeed() * 2) * updateTime);
									}

								}
								/* If the timer is 0 then it will create the shell*/
								if (this->Timer <= 0)
								{
									CVector3 TurretRot;
									this->TurretWorldMatrix.DecomposeAffineEuler(NULL, &TurretRot, NULL);
									CMatrix4x4 NewMatrix = Matrix(2) * Matrix();
									Timer = 1.0f;
									EntityManager.CreateTemplate("Projectile", "Shell Type 1", "Bullet.x");
									EntityManager.CreateShell("Shell Type 1", GetName(), this->TurretWorldMatrix.Position(), TurretRot);
									++this->ShootsFired;
									Fired = true;
									m_State = Evade;
								}

							}
							else
							{
								m_State = Evade;
								Timer = 1.0f;
							}
						}
						else
						{
							m_State = Evade;
							Timer = 1.0f;
						}
					}
					else
					{
						m_State = Evade;
						Timer = 1.0f;
					}
				}
				else
				{
					m_State = Evade;
					Timer = 1.0f;
				}
			}
			else
			{
				m_State = Ammo;
				Timer = 1.0f;
			}

			/* If the tank is hit it will send this message out to all the other tanks on its team */
			if (msg.type == Msg_Hit)
			{
				EntityManager.BeginEnumEntities("", "", "Tank");
				CEntity* entity = EntityManager.EnumEntity();
				while (entity != 0)
				{
					CTankEntity* TankEntity = static_cast<CTankEntity*>(entity);
					if (TankEntity->GetTeam() == this->GetTeam())
					{
						if (TankEntity != this && TankEntity->GetShootsFired() != 10 && TankEntity->m_State != TankEntity->Dead &&
							TankEntity->m_State != TankEntity->Aim && TankEntity->m_State != TankEntity->Ammo && TankEntity->m_State != TankEntity->Health)
						{
							TEntityUID UID = TankEntity->GetUID();
							msg.type = Msg_Help;
							//msg.from = SystemUID;
							Messenger.SendMessage(UID, msg);
						}
					}
					entity = EntityManager.EnumEntity();
				}
				EntityManager.EndEnumEntities();
			}

		}
		/* This is used when the tanks need ammo, takes elements from other states so look above ^ */
		if (m_State == Ammo)
		{

			EntityManager.BeginEnumEntities("", "", "AmmoCreate");
			CEntity* entity = EntityManager.EnumEntity();
			if (entity != NULL)
			{
				this->targetPos = entity->Position();
				CMatrix4x4 BodyMatrix = Matrix(0) * Matrix(1);
				CVector3 Facing = -BodyMatrix.ZAxis();
				CVector3 DistanceVect = Matrix().Position() - targetPos;
				DistanceVect.Normalise();
				if (m_Speed < m_TankTemplate->GetMaxSpeed())
				{
					m_Speed += m_TankTemplate->GetAcceleration();
				}
				float Angle = AngleMath(BodyMatrix, Facing, DistanceVect);
				float DotProduct = Dot(DistanceVect, BodyMatrix.XAxis());
				if (Angle < 3.0f)
				{
					Matrix().FaceTarget(this->targetPos);
					Matrix().MoveLocalZ(m_Speed * updateTime);
				}
				else
				{
					if (DotProduct > 0.0f)
					{
						Matrix().MoveLocalZ(m_TankTemplate->GetTurnSpeed() * updateTime);
						Matrix().RotateLocalY((-m_TankTemplate->GetTurnSpeed() / 2) * updateTime);
					}
					if (DotProduct < -0.0f)
					{
						Matrix().MoveLocalZ(m_TankTemplate->GetTurnSpeed() * updateTime);
						Matrix().RotateLocalY((m_TankTemplate->GetTurnSpeed() / 2) * updateTime);
					}
				}
				if (SphereToSphere(Matrix().GetPosition(), this->targetPos))
				{
					AmmoEntity* AE = static_cast<AmmoEntity*>(entity);
					this->ShootsFired = 0;
					AE->PickedUp = true;
					m_State = Patrol;
				}
			}
			else
			{
				m_State = Patrol;
			}
			if (msg.type == Msg_Hit)
			{
				EntityManager.BeginEnumEntities("", "", "Tank");
				CEntity* entity = EntityManager.EnumEntity();
				while (entity != 0)
				{
					CTankEntity* TankEntity = static_cast<CTankEntity*>(entity);
					if (TankEntity->GetTeam() == this->GetTeam())
					{
						if (TankEntity != this && TankEntity->GetShootsFired() != 10 && TankEntity->m_State != TankEntity->Dead &&
							TankEntity->m_State != TankEntity->Aim && TankEntity->m_State != TankEntity->Ammo && TankEntity->m_State != TankEntity->Health)
						{
							TEntityUID UID = TankEntity->GetUID();
							msg.type = Msg_Help;
							//msg.from = SystemUID;
							Messenger.SendMessage(UID, msg);
						}
					}
					entity = EntityManager.EnumEntity();
				}
				EntityManager.EndEnumEntities();
			}
		}
		/* This is the sames as the ammo create but with health instead, see above ^*/
		if (m_State == Health)
		{
			EntityManager.BeginEnumEntities("", "", "HealthCreate");
			CEntity* entity = EntityManager.EnumEntity();
			if (entity != NULL)
			{
				this->targetPos = entity->Position();
				CMatrix4x4 BodyMatrix = Matrix(0) * Matrix(1);
				CVector3 Facing = -BodyMatrix.ZAxis();
				CVector3 DistanceVect = Matrix().Position() - targetPos;
				DistanceVect.Normalise();
				if (m_Speed < m_TankTemplate->GetMaxSpeed())
				{
					m_Speed += m_TankTemplate->GetAcceleration();
				}
				float Angle = AngleMath(BodyMatrix, Facing, DistanceVect);
				float DotProduct = Dot(DistanceVect, BodyMatrix.XAxis());
				if (Angle < 3.0f)
				{
					Matrix().FaceTarget(this->targetPos);
					Matrix().MoveLocalZ(m_Speed * updateTime);
				}
				else
				{
					if (DotProduct > 0.0f)
					{
						Matrix().MoveLocalZ(m_TankTemplate->GetTurnSpeed() * updateTime);
						Matrix().RotateLocalY((-m_TankTemplate->GetTurnSpeed() / 2) * updateTime);
					}
					if (DotProduct < -0.0f)
					{
						Matrix().MoveLocalZ(m_TankTemplate->GetTurnSpeed() * updateTime);
						Matrix().RotateLocalY((m_TankTemplate->GetTurnSpeed() / 2) * updateTime);
					}
				}
				if (SphereToSphere(Matrix().GetPosition(), this->targetPos))
				{
					AmmoEntity* AE = static_cast<AmmoEntity*>(entity);
					this->m_HP += 50;
					AE->PickedUp = true;
					m_State = Patrol;
				}
			}
			else
			{
				m_State = Patrol;
			}
			if (msg.type == Msg_Hit)
			{
				EntityManager.BeginEnumEntities("", "", "Tank");
				CEntity* entity = EntityManager.EnumEntity();
				while (entity != 0)
				{
					CTankEntity* TankEntity = static_cast<CTankEntity*>(entity);
					if (TankEntity->GetTeam() == this->GetTeam())
					{
						if (TankEntity != this && TankEntity->GetShootsFired() != 10 && TankEntity->m_State != TankEntity->Dead &&
							TankEntity->m_State != TankEntity->Aim && TankEntity->m_State != TankEntity->Ammo && TankEntity->m_State != TankEntity->Health)
						{
							TEntityUID UID = TankEntity->GetUID();
							msg.type = Msg_Help;
							//msg.from = SystemUID;
							Messenger.SendMessage(UID, msg);
						}
					}
					entity = EntityManager.EnumEntity();
				}
				EntityManager.EndEnumEntities();
			}
		}
		if (m_State == Patrol)
		{
			if (PatrolPointer == PatrolList.size())
			{
				PatrolPointer = 0;
			}
			targetPos = PatrolList.at(PatrolPointer);
			//m_Speed = 10.0f;
			CMatrix4x4 BodyMatrix = Matrix(1) * Matrix();
			CVector3 Facing = -BodyMatrix.ZAxis();
			if (SphereToSphere(BodyMatrix.Position(), targetPos))
			{
				if (PatrolPointer == PatrolList.size())
				{
					PatrolPointer = 0;
				}
				targetPos = PatrolList.at(PatrolPointer);
				++PatrolPointer;
			}
			if (m_Speed < m_TankTemplate->GetMaxSpeed())
			{
				m_Speed += m_TankTemplate->GetAcceleration();
			}
			CVector3 DistanceVect = Matrix().Position() - targetPos;
			DistanceVect.Normalise();
			float Angle = AngleMath(BodyMatrix, Facing, DistanceVect);
			float DotProduct = Dot(DistanceVect, BodyMatrix.XAxis());
			if (Angle < 3.0f)
			{
				Matrix().FaceTarget(targetPos);
				Matrix().MoveLocalZ(m_Speed * updateTime);
			}
			else
			{
				if (DotProduct > 0.0f)
				{
					Matrix().MoveLocalZ(m_TankTemplate->GetTurnSpeed() * updateTime);
					Matrix().RotateLocalY(-(m_TankTemplate->GetTurnSpeed() / 2) * updateTime);
				}
				if (DotProduct < -0.0f)
				{
					Matrix().MoveLocalZ(m_TankTemplate->GetTurnSpeed() * updateTime);
					Matrix().RotateLocalY((m_TankTemplate->GetTurnSpeed() / 2) * updateTime);
				}
			}
			UpdateTankTargets();
			for (int i = 0; i < m_Target.size(); i++)
			{
				CEntity* Tank = EntityManager.GetEntity(this->m_Target.at(i));
				CTankEntity* TankEntity = static_cast<CTankEntity*>(Tank);
				if (TankEntity->m_State != TankEntity->Dead)
				{
					UpdateTankData(this->m_Target.at(i));
					Angle = AngleMath(this->TurretWorldMatrix, this->TankFacingVector, this->DistanceVector);
					if (Angle < 15.0f && !LineOfSight(this->TankFacingVector, Matrix(), TankTarget))
					{
						SavedEnemyIndex = i;
						m_State = Aim;

					}

				}
			}
			if (m_Target.size() == 0)
			{
				Matrix(2).RotateLocalY(m_TankTemplate->GetTurretTurnSpeed() * updateTime);
			}
			Matrix(2).RotateLocalY(m_TankTemplate->GetTurretTurnSpeed() * updateTime);
			if (msg.type == Msg_Hit)
			{
				EntityManager.BeginEnumEntities("", "", "Tank");
				CEntity* entity = EntityManager.EnumEntity();
				while (entity != 0)
				{
					CTankEntity* TankEntity = static_cast<CTankEntity*>(entity);
					if (TankEntity->GetTeam() == this->GetTeam())
					{
						if (TankEntity != this && TankEntity->GetShootsFired() != 10 && TankEntity->m_State != TankEntity->Dead &&
							TankEntity->m_State != TankEntity->Aim && TankEntity->m_State != TankEntity->Ammo && TankEntity->m_State != TankEntity->Health)
						{
							TEntityUID UID = TankEntity->GetUID();
							msg.type = Msg_Help;
							//msg.from = SystemUID;
							Messenger.SendMessage(UID, msg);
						}
					}
					entity = EntityManager.EnumEntity();
				}
				EntityManager.EndEnumEntities();
			}
		}

		if (m_State == Dead)
		{
			if (DeathTimer >= 0)
			{
				Matrix(2).MoveY(10 * updateTime);
				Matrix(2).RotateZ(10 * updateTime);
				DeathTimer -= updateTime;
			}
			else
			{
				if (DeathTimer2 <= 1.0f)
				{
					Matrix(2).MoveY(-10 * updateTime);
					Matrix(2).RotateZ(-10 * updateTime);
					DeathTimer2 += updateTime;
				}
				else
				{
					return false;
				}
			}
		}

		if (this->m_HP <= 0)
		{
			m_State = Dead;
		}
		return true; // Don't destroy the entity
	}


} // namespa
