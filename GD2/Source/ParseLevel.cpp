///////////////////////////////////////////////////////////
//  CParseLevel.cpp
//  A class to parse and setup a level (entity templates
//  and instances) from an XML file
//  Created on:      30-Jul-2005 14:40:00
//  Original author: LN
///////////////////////////////////////////////////////////

#include "BaseMath.h"
#include "Entity.h"
//#include "MoveComponent.h"
//#include "TargetComponent.h"
#include "ParseLevel.h"

namespace gen
{

	/*---------------------------------------------------------------------------------------------
		Constructors / Destructors
	---------------------------------------------------------------------------------------------*/

	// Constructor initialises state variables
	CParseLevel::CParseLevel(CEntityManager* entityManager)
	{
		// Take copy of entity manager for creation
		m_EntityManager = entityManager;

		// File state
		m_CurrentSection = None;

		// Template state
		m_TemplateType = "";
		m_TemplateName = "";
		m_TemplateMesh = "";
		m_TemplateHP = 0;
		m_TemplateMaxSpeed = 0.0f;
		m_TemplateAcceleration = 0.0f;
		m_TemplateTurnSpeed = 0.0f;

		// Entity state
		m_EntityType = "";
		m_EntityName = "";
		m_Pos = CVector3::kOrigin;
		m_PatrolPoints;
		m_Rot = CVector3::kOrigin;
		m_Scale = CVector3(1.0f, 1.0f, 1.0f);
	}


	/*---------------------------------------------------------------------------------------------
		Callback Functions
	---------------------------------------------------------------------------------------------*/

	// Callback function called when the parser meets the start of a new element (the opening tag).
	// The element name is passed as a string. The attributes are passed as a list of (C-style)
	// string pairs: attribute name, attribute value. The last attribute is marked with a null name
	void CParseLevel::StartElt(const string& eltName, SAttribute* attrs)
	{
		// Open major file sections
		if (eltName == "Templates")
		{
			m_CurrentSection = Templates;
		}
		else if (eltName == "Entities")
		{
			m_CurrentSection = Entities;
		}

		// Different parsing depending on section currently being read
		switch (m_CurrentSection)
		{
		case Templates:
			TemplatesStartElt(eltName, attrs); // Parse template start elements
			break;
		case Entities:
			EntitiesStartElt(eltName, attrs); // Parse entity start elements
			break;
		}
	}

	// Callback function called when the parser meets the end of an element (the closing tag). The
	// element name is passed as a string
	void CParseLevel::EndElt(const string& eltName)
	{
		// Close major file sections
		if (eltName == "Templates" || eltName == "Entities")
		{
			m_CurrentSection = None;
		}

		// Different parsing depending on section currently being read
		switch (m_CurrentSection)
		{
		case Templates:
			TemplatesEndElt(eltName); // Parse template end elements
			break;
		case Entities:
			EntitiesEndElt(eltName); // Parse entity end elements
			break;
		}
	}


	/*---------------------------------------------------------------------------------------------
		Section Parsing
	---------------------------------------------------------------------------------------------*/

	// Called when the parser meets the start of an element (opening tag) in the templates section
	void CParseLevel::TemplatesStartElt(const string& eltName, SAttribute* attrs)
	{
		if (eltName == "EntityTemplate")
		{
			m_TemplateType = GetAttribute(attrs, "Type");
			// Started reading a new entity template - get type, name and mesh
			if (m_TemplateType == "Tank")
			{
				// Get additional attributes held in ship template tags
				m_TemplateType = GetAttribute(attrs, "Type");
				m_TemplateName = GetAttribute(attrs, "Name");
				m_TemplateMesh = GetAttribute(attrs, "Mesh");
				m_TemplateHP = GetAttributeInt(attrs, "MaxHP");
				m_TemplateMaxSpeed = GetAttributeFloat(attrs, "MaxSpeed");
				m_TemplateAcceleration = GetAttributeFloat(attrs, "Acceleration");
				m_TemplateTurnSpeed = GetAttributeFloat(attrs, "TurnSpeed");
				m_TemplateTurretTurnSpeed = GetAttributeFloat(attrs, "TurretTurnSpeed");
				m_TemplateShellDamage = GetAttributeFloat(attrs, "ShellDamage");
				m_EntityManager->CreateTankTemplate(m_TemplateType, m_TemplateName, m_TemplateMesh, m_TemplateMaxSpeed, m_TemplateAcceleration,
					m_TemplateTurnSpeed, m_TemplateTurretTurnSpeed, m_TemplateHP, m_TemplateShellDamage);
			}
			else if (m_TemplateType == "AmmoCreate")
			{
				m_TemplateType = GetAttribute(attrs, "Type");
				m_TemplateName = GetAttribute(attrs, "Name");
				m_TemplateMesh = GetAttribute(attrs, "Mesh");
				m_EntityManager->CreateTemplate(m_TemplateType, m_TemplateName, m_TemplateMesh);
			}
			else if (m_TemplateType == "HealthCreate")
			{
				m_TemplateType = GetAttribute(attrs, "Type");
				m_TemplateName = GetAttribute(attrs, "Name");
				m_TemplateMesh = GetAttribute(attrs, "Mesh");
				m_EntityManager->CreateTemplate(m_TemplateType, m_TemplateName, m_TemplateMesh);
			}
			else
			{
				// Get attributes held in the tag
				m_TemplateType = GetAttribute(attrs, "Type");
				m_TemplateName = GetAttribute(attrs, "Name");
				m_TemplateMesh = GetAttribute(attrs, "Mesh");
				m_EntityManager->CreateTemplate(m_TemplateType, m_TemplateName, m_TemplateMesh);
			}
		}
	}

	// Called when the parser meets the end of an element (closing tag) in the templates section
	void CParseLevel::TemplatesEndElt(const string& eltName)
	{
		// Nothing to do
	}


	// Called when the parser meets the start of an element (opening tag) in the entities section
	void CParseLevel::EntitiesStartElt(const string& eltName, SAttribute* attrs)
	{
		// Started reading a new entity - get type and name
		if (eltName == "Entity")
		{
			m_EntityType = GetAttribute(attrs, "Type");
			m_EntityName = GetAttribute(attrs, "Name");

			// Create a new entity - add components during parsing and initialise position on closing tag
			TEntityUID entityUID = m_EntityManager->CreateEntity(m_EntityType, m_EntityName);
			m_Entity = m_EntityManager->GetEntity(entityUID);

			// Set default positions
			m_Pos = CVector3::kOrigin;
			m_Rot = CVector3::kOrigin;
			m_Scale = CVector3(1.0f, 1.0f, 1.0f);
		}
		else if (eltName == "TankEntity")
		{
			m_EntityType = GetAttribute(attrs, "Type");
			m_EntityName = GetAttribute(attrs, "Name");
			m_EntityTeam = GetAttributeInt(attrs, "Team");
			TEntityUID entityUID = m_EntityManager->CreateTank(m_EntityType, m_EntityTeam, m_EntityName, m_PatrolPoints ,m_Pos, m_Rot);
			m_PatrolPoints.clear();
			m_Entity = m_EntityManager->GetEntity(entityUID);

			m_Pos = CVector3::kOrigin;
			m_Rot = CVector3::kOrigin;
			m_Scale = CVector3(1.0f, 1.0f, 1.0f);

		}
		else if (eltName == "Loop")
		{
			m_TemplateType = GetAttribute(attrs, "Type");
			m_TemplateName = GetAttribute(attrs, "Name");
			int m_TemplateAmount = GetAttributeInt(attrs, "Amount");
			float m_TemplateX = GetAttributeFloat(attrs, "X");
			float m_TemplateY = GetAttributeFloat(attrs, "Y");
			float m_TemplateZ = GetAttributeFloat(attrs, "Z");
			float m_TemplateMaxX = GetAttributeFloat(attrs, "MaxX");
			float m_TemplateMaxY = GetAttributeFloat(attrs, "MaxY");
			float m_TemplateMaxZ = GetAttributeFloat(attrs, "MaxZ");

			for (int i = 0; i < m_TemplateAmount; i++)
			{
				float m_XPos, m_YPos, m_ZPos;
				TEntityUID entityUID = m_EntityManager->CreateEntity(m_TemplateType, m_TemplateName);
				m_Entity = m_EntityManager->GetEntity(entityUID);

				m_Pos.x = Random(m_TemplateX, m_TemplateMaxX);
				//m_YPos = Random(m_TemplateY, m_TemplateMaxY);
				m_Pos.z = Random(m_TemplateZ, m_TemplateMaxZ);
				m_Entity->Matrix().MakeAffineEuler(m_Pos, m_Rot, kZXY, m_Scale);

			}
		}
		// Started reading an entity position - get X,Y,Z
		else if (eltName == "Position")
		{
			m_Pos.x = GetAttributeFloat(attrs, "X");
			m_Pos.y = GetAttributeFloat(attrs, "Y");
			m_Pos.z = GetAttributeFloat(attrs, "Z");
		}
		else if (eltName == "PatrolPoint")
		{
			CVector3 CurrentPatrolPoint;
			CurrentPatrolPoint.x = GetAttributeFloat(attrs, "X");
			CurrentPatrolPoint.y = GetAttributeFloat(attrs, "Y");
			CurrentPatrolPoint.z = GetAttributeFloat(attrs, "Z");
			m_PatrolPoints.push_back(CurrentPatrolPoint);
		}

		// Started reading an entity rotation - get X,Y,Z
		else if (eltName == "Rotation")
		{
			if (GetAttribute(attrs, "Radians") == "true")
			{
				m_Rot.x = GetAttributeFloat(attrs, "X");
				m_Rot.y = GetAttributeFloat(attrs, "Y");
				m_Rot.z = GetAttributeFloat(attrs, "Z");
			}
			else
			{
				m_Rot.x = ToRadians(GetAttributeFloat(attrs, "X"));
				m_Rot.y = ToRadians(GetAttributeFloat(attrs, "Y"));
				m_Rot.z = ToRadians(GetAttributeFloat(attrs, "Z"));
			}
		}

		// Started reading an entity scale - get X,Y,Z
		else if (eltName == "Scale")
		{
			m_Scale.x = GetAttributeFloat(attrs, "X");
			m_Scale.y = GetAttributeFloat(attrs, "Y");
			m_Scale.z = GetAttributeFloat(attrs, "Z");
		}

		// Randomising an entity position - get X,Y,Z amounts and randomise
		else if (eltName == "Randomise")
		{
			float randomX = GetAttributeFloat(attrs, "X") * 0.5f;
			float randomY = GetAttributeFloat(attrs, "Y") * 0.5f;
			float randomZ = GetAttributeFloat(attrs, "Z") * 0.5f;
			m_Pos.x += Random(-randomX, randomX);
			m_Pos.y += Random(-randomY, randomY);
			m_Pos.z += Random(-randomZ, randomZ);
		}

		// Component to add to entity
		else if (eltName == "Component")
		{
			ComponentsStartElt(GetAttribute(attrs, "Type"), attrs);
		}
	}

	// Called when the parser meets the end of an element (closing tag) in the entities section
	void CParseLevel::EntitiesEndElt(const string& eltName)
	{
		if (eltName == "Entity")
		{
			m_Entity->Matrix().MakeAffineEuler(m_Pos, m_Rot, kZXY, m_Scale);
		}
		if(eltName == "TankEntity")
		{
		m_Entity->Matrix().MakeAffineEuler(m_Pos, m_Rot, kZXY, m_Scale);
		}

	}

	//****************************************************************************/
	//  Component Code
	//****************************************************************************/

	// Called when the parser meets the start of an element (opening tag) of a entity component
	void CParseLevel::ComponentsStartElt(const string& typeName, SAttribute* attrs)
	{

	}
	//
	//****************************************************************************/


} // namespace gen
