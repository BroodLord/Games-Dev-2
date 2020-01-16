/*******************************************
	TankEntity.h

	Tank entity template and entity classes
********************************************/

#pragma once

#include <string>
using namespace std;

#include "Defines.h"
#include "CVector3.h"
#include "Entity.h"

namespace gen
{

	/*-----------------------------------------------------------------------------------------
	-------------------------------------------------------------------------------------------
		Tank Template Class
	-------------------------------------------------------------------------------------------
	-----------------------------------------------------------------------------------------*/

	// A tank template inherits the type, name and mesh from the base template and adds further
	// tank specifications
	class CTankTemplate : public CEntityTemplate
	{
		/////////////////////////////////////
		//	Constructors/Destructors
	public:
		// Tank entity template constructor sets up the tank specifications - speed, acceleration and
		// turn speed and passes the other parameters to construct the base class
		CTankTemplate
		(
			const string& type, const string& name, const string& meshFilename,
			TFloat32 maxSpeed, TFloat32 acceleration, TFloat32 turnSpeed,
			TFloat32 turretTurnSpeed, TUInt32 maxHP, TUInt32 shellDamage
		) : CEntityTemplate(type, name, meshFilename)
		{
			// Set tank template values
			m_MaxSpeed = maxSpeed;
			m_Acceleration = acceleration;
			m_TurnSpeed = turnSpeed;
			m_TurretTurnSpeed = turretTurnSpeed;
			m_MaxHP = maxHP;
			m_ShellDamage = shellDamage;
		}

		// No destructor needed (base class one will do)


	/////////////////////////////////////
	//	Public interface
	public:

		/////////////////////////////////////
		//	Getters

		TFloat32 GetMaxSpeed()
		{
			return m_MaxSpeed;
		}

		TFloat32 GetAcceleration()
		{
			return m_Acceleration;
		}

		TFloat32 GetTurnSpeed()
		{
			return m_TurnSpeed;
		}

		TFloat32 GetTurretTurnSpeed()
		{
			return m_TurretTurnSpeed;
		}

		TInt32 GetMaxHP()
		{
			return m_MaxHP;
		}

		TInt32 GetShellDamage()
		{
			return m_ShellDamage;
		}


		/////////////////////////////////////
		//	Private interface
	private:

		// Common statistics for this tank type (template)
		TFloat32 m_MaxSpeed;        // Maximum speed for this kind of tank
		TFloat32 m_Acceleration;    // Acceleration  -"-
		TFloat32 m_TurnSpeed;       // Turn speed    -"-
		TFloat32 m_TurretTurnSpeed; // Turret turn speed    -"-

		TUInt32  m_MaxHP;           // Maximum (initial) HP for this kind of tank
		TUInt32  m_ShellDamage;     // HP damage caused by shells from this kind of tank
	};



	/*-----------------------------------------------------------------------------------------
	-------------------------------------------------------------------------------------------
		Tank Entity Class
	-------------------------------------------------------------------------------------------
	-----------------------------------------------------------------------------------------*/

	// A tank entity inherits the ID/positioning/rendering support of the base entity class
	// and adds instance and state data. It overrides the update function to perform the tank
	// entity behaviour
	// The shell code performs very limited behaviour to be rewritten as one of the assignment
	// requirements. You may wish to alter other parts of the class to suit your game additions
	// E.g extra member variables, constructor parameters, getters etc.
	class CTankEntity : public CEntity
	{
		/////////////////////////////////////
		//	Constructors/Destructors
	public:
		// Tank constructor intialises tank-specific data and passes its parameters to the base
		// class constructor
		CTankEntity
		(
			CTankTemplate* tankTemplate,
			TEntityUID      UID,
			TUInt32         team,
			const string& name,
			vector<CVector3> patrolPoints,
			const CVector3& position = CVector3::kOrigin,
			const CVector3& rotation = CVector3(0.0f, 0.0f, 0.0f),
			const CVector3& scale = CVector3(1.0f, 1.0f, 1.0f)
		);

		// No destructor needed


	/////////////////////////////////////
	//	Public interface
	public:

		// States available for a tank - placeholders for shell code
		enum EState
		{
			Inactive,
			Patrol,
			Ammo,
			Health,
			Dead,
			Aim,
			Evade
		};
		EState   m_State; // Current state
		/////////////////////////////////////
		// Getters

		TFloat32 GetSpeed()
		{
			return m_Speed;
		}
		TInt32 GetHP()
		{
			return m_HP;
		}
		TInt32 GetFollowed()
		{
			return BeingFollowed;
		}
		void SetFollowed(bool Set)
		{
			BeingFollowed = Set;
		}
		bool GetSelected()
		{
			return Selected;
		}
		void SetSelected(bool Set)
		{
			Selected = Set;
		}
		TInt32 GetShootsFired()
		{
			return ShootsFired;
		}
		void SetTargetPos(CVector3 Set)
		{
			RandomPos = Set;
		}
		CVector3 GetTargetPos()
		{
			return RandomPos;
		}
		vector<CVector3> GetPatrolList()
		{
			return PatrolList;
		}
		void SetPatrolList(vector<CVector3> PL)
		{
			PatrolList.clear();
			for (int i = 0; i < PL.size(); i++)
			{
				PatrolList.push_back(PL.at(i));
			}
		}
		int GetShellDamageTE()
		{
			return m_TankTemplate->GetShellDamage();
		}
		void SetTeam(int Set)
		{
			m_Team = Set;
		}
		string GetState()
		{
			return to_string(m_State);
		}
		string GetStateToString()
		{
			switch (m_State)
			{
			case gen::CTankEntity::Inactive:
				return "Inactive";
				break;
			case gen::CTankEntity::Patrol:
				return "Patrol";
				break;
			case gen::CTankEntity::Ammo:
				return "Ammo";
				break;
			case gen::CTankEntity::Health:
				return "Health";
				break;
			case gen::CTankEntity::Dead:
				return "Dead";
				break;
			case gen::CTankEntity::Aim:
				return "Aim";
				break;
			case gen::CTankEntity::Evade:
				return "Evade";
				break;
			//case gen::CTankEntity::Stop:
			//	return "Stop";
			//	break;
			//case gen::CTankEntity::Go:
			//	return "Go";
			//	break;
			}
		}
		int GetTeam()
		{
			return m_Team;
		}

		/////////////////////////////////////
		// Update

		// Update the tank - performs tank message processing and behaviour
		// Return false if the entity is to be destroyed
		// Keep as a virtual function in case of further derivation
		virtual bool Update(TFloat32 updateTime);
		void UpdateTankData(int Index);
		void UpdateTankTargets();

		/////////////////////////////////////
		//	Private interface
	private:

		/////////////////////////////////////
		// Types


		/////////////////////////////////////
		// Data

		// The template holding common data for all tank entities
		CTankTemplate* m_TankTemplate;

		// Tank data
		TUInt32  m_Team;  // Team number for tank (to know who the enemy is)
		TFloat32 m_Speed; // Current speed (in facing direction)
		TInt32   m_HP;    // Current hit points for the tank
		TInt32 ShootsFired = 0;
		vector<TEntityUID> m_Target;
		CEntity* TankTarget;
		CTankEntity* TargetTank;
		CMatrix4x4 TurretWorldMatrix;
		CVector3 TankFacingVector;
		CVector3 DistanceVector;
		vector<CVector3> PatrolList;
		int PatrolPointer = 0;
		CVector3 targetPos;
		float Timer = 1.0f;
		float DeathTimer = 1.0f;
		float DeathTimer2 = 0;
		int SavedEnemyIndex;
		CVector3 RandomPos = CVector3(Random( -20, 20), 0.5, Random(-20, 20));
		bool AtTarget = false;
		float Angle;
		bool Fired = false;
		bool Picked = false;
		bool BeingFollowed = false;
		bool Selected = true;

		// Tank state
		TFloat32 m_Timer; // A timer used in the example update function
	};


} // namespacenamespace gen
