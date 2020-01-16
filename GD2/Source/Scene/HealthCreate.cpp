#include "HealthCreate.h"
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


	/*-----------------------------------------------------------------------------------------
	-------------------------------------------------------------------------------------------
		Shell Entity Class
	-------------------------------------------------------------------------------------------
	-----------------------------------------------------------------------------------------*/

	// Shell constructor intialises shell-specific data and passes its parameters to the base
	// class constructor
	HealthCreate::HealthCreate
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

	bool HealthCreate::Update(TFloat32 updateTime)
	{
		if (PickedUp != true)
		{
			Matrix().RotateLocalY(2 * updateTime);
			if (Matrix().Position().y > 1.0f)
			{
				this->Matrix().MoveLocalY(-5 * updateTime);
			}
			else
			{
				Grounded = true;
				if (Grounded == true)
				{
					SMessage msg;
					EntityManager.BeginEnumEntities("", "", "Tank");
					CEntity* entity = EntityManager.EnumEntity();
					while (entity != 0)
					{
						TEntityUID UID = entity->GetUID();
						//TanksUIDs[i] = UID;
						msg.type = Msg_Health;
						msg.from = SystemUID;
						Messenger.SendMessage(UID, msg);
						entity = EntityManager.EnumEntity();
					}
					EntityManager.EndEnumEntities();

					if (DeathTimer < 0)
					{
						return false;
					}
					else
					{
						DeathTimer -= updateTime;
					}
				}
			}
		}
		else
		{
			return false;
		}
		return true;
	}


} // namespace genge