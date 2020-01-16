/*******************************************
	ShellEntity.cpp

	Shell entity class
********************************************/

#include "ShellEntity.h"
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
	// Will be needed to implement the required shell behaviour in the Update function below
	extern TEntityUID GetTankUID(int team);
	vector<CTankEntity*> TargetList;


	/*-----------------------------------------------------------------------------------------
	-------------------------------------------------------------------------------------------
		Shell Entity Class
	-------------------------------------------------------------------------------------------
	-----------------------------------------------------------------------------------------*/

	// Shell constructor intialises shell-specific data and passes its parameters to the base
	// class constructor
	CShellEntity::CShellEntity
	(
		CEntityTemplate* entityTemplate,
		TEntityUID       UID,
		const string& name /*=""*/,
		const CVector3& position /*= CVector3::kOrigin*/,
		const CVector3& rotation /*= CVector3( 0.0f, 0.0f, 0.0f )*/,
		const CVector3& scale /*= CVector3( 1.0f, 1.0f, 1.0f )*/
	) : CEntity(entityTemplate, UID, name, position, rotation, scale)
	{


	}
	// Update the shell - controls its behaviour. The shell code is empty, it needs to be written as
	// one of the assignment requirements
	// Return false if the entity is to be destroyed

	void EnemyList(string BulletName)
	{
		int Team = 0;
		vector<CTankEntity*> EL;
		EntityManager.BeginEnumEntities("", "", "Tank");
		CEntity* entity = EntityManager.EnumEntity();
		while (entity != 0)
		{
			if (entity->GetName() == BulletName)
			{
				CTankEntity* CT = static_cast<CTankEntity*>(entity);
				Team = CT->GetTeam();
				break;
			}
			entity = EntityManager.EnumEntity();
		}
		EntityManager.EndEnumEntities();

		EntityManager.BeginEnumEntities("", "", "Tank");
		entity = EntityManager.EnumEntity();
		while (entity != 0)
		{
			CTankEntity* CT = static_cast<CTankEntity*>(entity);
			if (CT->GetTeam() != Team)
			{
				if (CT != NULL)
				{
					EL.push_back(CT);
				}
			}
			entity = EntityManager.EnumEntity();
		}
		EntityManager.EndEnumEntities();
		TargetList = EL;
	}

	bool CShellEntity::Update(TFloat32 updateTime)
	{
		EnemyList(GetName());
		static float Timer = 2.0f;
		if (Timer >= 0)
		{
			SMessage msg;
			msg.type = Msg_Hit;
			msg.from = SystemUID;
			Timer -= updateTime;
			EntityManager.BeginEnumEntities("", "", "Tank");
			CEntity* entity = EntityManager.EnumEntity();
			while (entity != 0)
			{
				if (this->GetName() == entity->GetName())
				{
					msg.from = entity->GetUID();
					CTankEntity* TE = static_cast<CTankEntity*>(entity);
					msg.damage = TE->GetShellDamageTE();
				}
				entity = EntityManager.EnumEntity();
			}
			EntityManager.EndEnumEntities();


			for (int i = 0; i < TargetList.size(); i++)
			{
				//TEntityUID TankUID = GetTankUID(i);
					CEntity* TankObject = EntityManager.GetEntity(TargetList[i]->GetUID());
					CTankEntity* TankEntity = static_cast<CTankEntity*>(TankObject);
					if (TankObject != NULL && TankEntity->m_State != TankEntity->Dead)
					{

						if (Matrix().Position().x > TankObject->Matrix().Position().x - 2 && Matrix().Position().x < TankObject->Matrix().Position().x + 2 &&
							Matrix().Position().y > TankObject->Matrix().Position().y - 4 && Matrix().Position().y < TankObject->Matrix().Position().y + 4 &&
							Matrix().Position().z > TankObject->Matrix().Position().z - 4 && Matrix().Position().z < TankObject->Matrix().Position().z + 4)
						{
							if (GetName() != TankObject->GetName())
							{
								Messenger.SendMessage(TankObject->GetUID(), msg);
								Timer = 2.0f;
								return false;
							}
						}
					}
				
			}
		}
		else
		{
			Timer = 2.0f;
			return false;
		}
		Matrix().MoveLocalZ(160 * updateTime);
		return true; // Placeholder
	}


} // namespace gengenen
