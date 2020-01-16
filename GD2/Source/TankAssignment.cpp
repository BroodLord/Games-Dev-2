/*******************************************
	TankAssignment.cpp

	Shell scene and game functions
********************************************/

#include <sstream>
#include <string>
using namespace std;

#include <d3d10.h>
#include <d3dx10.h>

#include "Defines.h"
#include "CVector3.h"
#include "Camera.h"
#include "Light.h"
#include "EntityManager.h"
#include "Messenger.h"
#include "ParseLevel.h"
#include "TankAssignment.h"

namespace gen
{
	vector<TEntityUID> TanksUIDs;
	vector<CTankEntity*> TankEntities;
	CTankEntity* SelectedTank;
	vector<CVector3> TeamOnePatrolList;
	vector<CVector3> TeamTwoPatrolList;
	CVector3 TankPoses[6];
	int NumTanks = 6;
	float AmmoTimer = 20.0f;
	float HealthTimer = 30.0f;
	bool SelectedTankBool = false;
	int SavedIndex = 0;
	//-----------------------------------------------------------------------------
	// Constants
	//-----------------------------------------------------------------------------

	// Control speed
	const float CameraRotSpeed = 2.0f;
	float CameraMoveSpeed = 80.0f;
	CEntity* NearestEntity = 0;

	// Amount of time to pass before calculating new average update time
	const float UpdateTimePeriod = 1.0f;


	//-----------------------------------------------------------------------------
	// Global system variables
	//-----------------------------------------------------------------------------

	// Get reference to global DirectX variables from another source file
	extern ID3D10Device* g_pd3dDevice;
	extern IDXGISwapChain* SwapChain;
	extern ID3D10DepthStencilView* DepthStencilView;
	extern ID3D10RenderTargetView* BackBufferRenderTarget;
	extern ID3DX10Font* OSDFont;

	// Actual viewport dimensions (fullscreen or windowed)
	extern TUInt32 ViewportWidth;
	extern TUInt32 ViewportHeight;


	// Current mouse position
	extern TUInt32 MouseX;
	extern TUInt32 MouseY;
	extern CVector2 MousePixel;

	extern TEntityUID GetTankUID(int Team);

	// Messenger class for sending messages to and between entities
	extern CMessenger Messenger;


	//-----------------------------------------------------------------------------
	// Global game/scene variables
	//-----------------------------------------------------------------------------

	// Entity manager
	CEntityManager EntityManager;
	CParseLevel LevelParser(&EntityManager);

	// Tank UIDs
	TEntityUID TankA;
	TEntityUID TankB;
	int Counter = 0;

	// Other scene elements
	const int NumLights = 2;
	CLight* Lights[NumLights];
	SColourRGBA AmbientLight;
	CCamera* MainCamera;

	// Sum of recent update times and number of times in the sum - used to calculate
	// average over a given time period
	float SumUpdateTimes = 0.0f;
	int NumUpdateTimes = 0;
	float AverageUpdateTime = -1.0f; // Invalid value at first


	//-----------------------------------------------------------------------------
	// Scene management
	//-----------------------------------------------------------------------------

	// Creates the scene geometry
	bool SceneSetup()
	{
		//////////////////////////////////////////////
		// Prepare render methods

		InitialiseMethods();
		LevelParser.ParseFile("Entities.xml");

		////////////////////////////////
		// Create tank entities
		// Type (template name), team number, tank name, position, rotation
		bool Team1Added = false;
		bool Team0Added = false;
		EntityManager.BeginEnumEntities("", "", "Tank");
		CEntity* entity = EntityManager.EnumEntity();
		while (entity != 0)
		{
			TEntityUID UID = entity->GetUID();
			TanksUIDs.push_back(UID);
			TankEntities.push_back(static_cast<CTankEntity*>(entity));
			if (TankEntities.at(Counter)->GetTeam() == 0)
			{
				if (!Team0Added)
				{
					TeamOnePatrolList = TankEntities.at(Counter)->GetPatrolList();
					Team0Added = true;
				}
			}
			else if (TankEntities.at(Counter)->GetTeam() == 1)
			{
				if (!Team1Added)
				{
					TeamTwoPatrolList = TankEntities.at(Counter)->GetPatrolList();
					Team1Added = true;
				}
			}
			entity = EntityManager.EnumEntity();
			++Counter;
		}
		EntityManager.EndEnumEntities();
		Counter = 0;

		/////////////////////////////
		// Camera / light setup

		// Set camera position and clip planes
		MainCamera = new CCamera(CVector3(0.0f, 30.0f, -100.0f), CVector3(ToRadians(15.0f), 0, 0));
		MainCamera->SetNearFarClip(1.0f, 20000.0f);

		// Sunlight and light in building
		Lights[0] = new CLight(CVector3(-5000.0f, 4000.0f, -10000.0f), SColourRGBA(1.0f, 0.9f, 0.6f), 15000.0f);
		Lights[1] = new CLight(CVector3(6.0f, 7.5f, 40.0f), SColourRGBA(1.0f, 0.0f, 0.0f), 1.0f);

		// Ambient light level
		AmbientLight = SColourRGBA(0.6f, 0.6f, 0.6f, 1.0f);

		return true;
	}


	// Release everything in the scene
	void SceneShutdown()
	{
		// Release render methods
		ReleaseMethods();

		// Release lights
		for (int light = NumLights - 1; light >= 0; --light)
		{
			delete Lights[light];
		}

		// Release camera
		delete MainCamera;

		// Destroy all entities
		EntityManager.DestroyAllEntities();
		EntityManager.DestroyAllTemplates();
	}


	//-----------------------------------------------------------------------------
	// Game Helper functions
	//-----------------------------------------------------------------------------

	// Get UID of tank A (team 0) or B (team 1)
	TEntityUID GetTankUID(int team)
	{
		return (team == 0) ? TankA : TankB;
	}


	//-----------------------------------------------------------------------------
	// Game loop functions
	//-----------------------------------------------------------------------------

	// Draw one frame of the scene
	void RenderScene(float updateTime)
	{
		// Setup the viewport - defines which part of the back-buffer we will render to (usually all of it)
		D3D10_VIEWPORT vp;
		vp.Width = ViewportWidth;
		vp.Height = ViewportHeight;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		g_pd3dDevice->RSSetViewports(1, &vp);

		// Select the back buffer and depth buffer to use for rendering
		g_pd3dDevice->OMSetRenderTargets(1, &BackBufferRenderTarget, DepthStencilView);

		// Clear previous frame from back buffer and depth buffer
		g_pd3dDevice->ClearRenderTargetView(BackBufferRenderTarget, &AmbientLight.r);
		g_pd3dDevice->ClearDepthStencilView(DepthStencilView, D3D10_CLEAR_DEPTH, 1.0f, 0);

		// Update camera aspect ratio based on viewport size - for better results when changing window size
		MainCamera->SetAspect(static_cast<TFloat32>(ViewportWidth) / ViewportHeight);

		// Set camera and light data in shaders
		MainCamera->CalculateMatrices();
		SetCamera(MainCamera);
		SetAmbientLight(AmbientLight);
		SetLights(&Lights[0]);

		// Render entities and draw on-screen text
		EntityManager.RenderAllEntities();
		RenderSceneText(updateTime);

		// Present the backbuffer contents to the display
		SwapChain->Present(0, 0);
	}


	// Render a single text string at the given position in the given colour, may optionally centre it
	void RenderText(const string& text, int X, int Y, float r, float g, float b, bool centre = false)
	{
		RECT rect;
		if (!centre)
		{
			SetRect(&rect, X, Y, 0, 0);
			OSDFont->DrawText(NULL, text.c_str(), -1, &rect, DT_NOCLIP, D3DXCOLOR(r, g, b, 1.0f));
		}
		else
		{
			SetRect(&rect, X - 100, Y, X + 100, 0);
			OSDFont->DrawText(NULL, text.c_str(), -1, &rect, DT_CENTER | DT_NOCLIP, D3DXCOLOR(r, g, b, 1.0f));
		}
	}

	// Render on-screen text each frame
	void RenderSceneText(float updateTime)
	{
		// Accumulate update times to calculate the average over a given period
		SumUpdateTimes += updateTime;
		++NumUpdateTimes;
		if (SumUpdateTimes >= UpdateTimePeriod)
		{
			AverageUpdateTime = SumUpdateTimes / NumUpdateTimes;
			SumUpdateTimes = 0.0f;
			NumUpdateTimes = 0;
		}

		// Write FPS text string and Key button Text
		stringstream outText;
		if (AverageUpdateTime >= 0.0f)
		{
			outText << "Frame Time: " << AverageUpdateTime * 1000.0f << "ms" << endl << "FPS:" << 1.0f / AverageUpdateTime;
			RenderText(outText.str(), 2, 2, 0.0f, 0.0f, 0.0f);
			RenderText(outText.str(), 0, 0, 1.0f, 1.0f, 0.0f);
			outText.str("");
			outText << "-----------------------------";
			RenderText(outText.str(), 2, 22, 0.0f, 0.0f, 0.0f);
			RenderText(outText.str(), 0, 20, 1.0f, 1.0f, 0.0f);
			outText.str("");
			outText << "Start: " << "Key_1" << endl << "Stop: " << "Key_2" << endl << "Chase Camera: " << "Key_3" << endl << "Chase Camera Exit: " << "Key_4" << endl
					<< "Mouse_RButton: " << " Pick Up Objects" << endl << "Mouse_LButton: " << "Click on Tank then a space in world to make" << endl 
					<<" it move there (Puts into Evade State)";
			RenderText(outText.str(), 2, 30, 0.0f, 0.0f, 0.0f);
			RenderText(outText.str(), 0, 28, 1.0f, 1.0f, 0.0f);
			outText.str("");
		}
		// Write FPS text string
		RenderText(outText.str(), 0, 0, 1.0f, 1.0f, 0.0f);
		outText.str("");

		int X, Y;
		NearestEntity = NULL;
		CVector2 entityPixel;
		float nearestDistance = 50;
		float pixelDistance;
		/*Loops through all tanks and finds the nearest the mouse*/
		EntityManager.BeginEnumEntities("", "", "Tank");
		CEntity* entity = EntityManager.EnumEntity();
		while (entity != 0)
		{
			if (MainCamera->PixelFromWorldPt(&entityPixel, entity->Position(), ViewportWidth, ViewportHeight))
			{
				pixelDistance = Distance(MousePixel, entityPixel);
				if (pixelDistance < nearestDistance)
				{
					NearestEntity = entity;
					//RenderText(outText.str(), 0, 0, 1.0f, 1.0f, 0.0f);
					nearestDistance = pixelDistance;
				}
			}
			entity = EntityManager.EnumEntity();
		}
		EntityManager.EndEnumEntities();
		/*88888888888888888888888888888888888888888888888888888888888888*/
		/*Loops through all tanks*/
		EntityManager.BeginEnumEntities("", "", "Tank");
		entity = EntityManager.EnumEntity();
		//CTankEntity* TankEntity = static_cast<CTankEntity*>(entity);
		while (entity != 0)
		{
			CVector2 pixelPt;
			/*When the key is hit, it will display additional infomation*/
			if (KeyHeld(Key_0))
			{
				//CVector2 pixelPt;
				if (MainCamera->PixelFromWorldPt(&pixelPt, entity->Position(), ViewportWidth, ViewportHeight))
				{
					
					CTankEntity* TankEntity = static_cast<CTankEntity*>(entity);
					outText << "HP:" << TankEntity->GetHP() << endl << "Team: " << TankEntity->GetTeam() << endl << "ShootsFired: " << TankEntity->GetShootsFired() << "/10" << endl << " State: " << TankEntity->GetStateToString().c_str();
					if (TankEntity->GetTeam() == 0)
					{
						RenderText(outText.str(), (int)pixelPt.x, (int)pixelPt.y, 0.2f, 1.0f, 0.2f, true);
					}
					else
					{
						RenderText(outText.str(), (int)pixelPt.x, (int)pixelPt.y, 1.0f, 0.2f, 0.2f, true);
					}
					if (SelectedTank != NULL)
					{
						if (SelectedTank->GetUID() == TankEntity->GetUID())
						{
							RenderText(outText.str(), (int)pixelPt.x, (int)pixelPt.y, 1.0f, 1.0f, 1.0f, true);
						}
					}
					outText.str("");
				}
			}
			/*If not pressed it will just display the name*/
			else if (MainCamera->PixelFromWorldPt(&pixelPt, entity->Position(), ViewportWidth, ViewportHeight))
			{
				outText << entity->Template()->GetName().c_str() << " " << entity->GetName().c_str();
				if (entity == NearestEntity)
				{
					RenderText(outText.str(), (int)pixelPt.x, (int)pixelPt.y, 1.0f, 1.0f, 1.0f, true);
				}
				else
				{
					CTankEntity* TankEntity = static_cast<CTankEntity*>(entity);
					if (TankEntity->GetTeam() == 0)
					{
						RenderText(outText.str(), (int)pixelPt.x, (int)pixelPt.y, 0.2f, 1.0f, 0.2f, true);
					}
					else
					{
						RenderText(outText.str(), (int)pixelPt.x, (int)pixelPt.y, 1.0f, 0.2f, 0.2f, true);
					}
					if (SelectedTank != NULL)
					{
						if (SelectedTank->GetUID() == TankEntity->GetUID())
						{
							RenderText(outText.str(), (int)pixelPt.x, (int)pixelPt.y, 1.0f, 1.0f, 1.0f, true);
						}
					}
				}
				outText.str("");
			}
			entity = EntityManager.EnumEntity();
		}
		EntityManager.EndEnumEntities();

		if (SelectedTankBool == false)
		{
			/*Will allow the user to select the tank*/
			if (KeyHit(Mouse_LButton) && NearestEntity != NULL)
			{
				CTankEntity* TankEntity = static_cast<CTankEntity*>(NearestEntity);
				if (TankEntity->m_State != TankEntity->Inactive)
				{
					SelectedTank = TankEntity;
					SelectedTankBool = true;
				}
			}
		}

		/*The will put the selected tank into the eveade state with a point given by the mouse*/
		if (SelectedTank != NULL)
		{
			if (SelectedTankBool == true)
			{
				outText.str("");
				if (KeyHit(Mouse_LButton))
				{
					CVector3 MousePointer = MainCamera->WorldPtFromPixel(MousePixel, ViewportWidth, ViewportHeight);
					CVector3 RayCast = Normalise(MousePointer - MainCamera->Position());
					CVector3 NewPos = MainCamera->Position() + ((-MainCamera->Position().y / RayCast.y) * RayCast);
					SelectedTank->m_State = SelectedTank->Evade;
					SelectedTank->SetTargetPos(NewPos);
					SelectedTank = NULL;
					SelectedTankBool = false;
				}
			}
		}

		/* The will run through all the scenery and allow it to be picked up apart from the floor*/
		EntityManager.BeginEnumEntities("", "", "Scenery");
		entity = EntityManager.EnumEntity();
		while (entity != 0)
		{
			if (MainCamera->PixelFromWorldPt(&entityPixel, entity->Position(), ViewportWidth, ViewportHeight))
			{
				if (entity->GetName() != "Floor")
				{
					pixelDistance = Distance(MousePixel, entityPixel);
					if (pixelDistance < nearestDistance)
					{
						NearestEntity = entity;
						//RenderText(outText.str(), 0, 0, 1.0f, 1.0f, 0.0f);
						nearestDistance = pixelDistance;
					}
				}
			}
			entity = EntityManager.EnumEntity();
		}
		EntityManager.EndEnumEntities();

		
		if (NearestEntity != NULL)
		{
			if (KeyHeld(Mouse_RButton))
			{
				CVector3 MousePointer = MainCamera->WorldPtFromPixel(MousePixel, ViewportWidth, ViewportHeight);
				CVector3 RayCast = Normalise(MousePointer - MainCamera->Position());
				CVector3 NewPos = MainCamera->Position() + ((-MainCamera->Position().y / RayCast.y) * RayCast);
				//CVector3 CameraPos = MainCamera->Position() + RayCast * 100;
				NearestEntity->Position().x = NewPos.x;
				NearestEntity->Position().y = NewPos.y;
				NearestEntity->Position().z = NewPos.z;
			}
		}
		/* This allows the ammoCreate to be picked up */
		EntityManager.BeginEnumEntities("", "", "AmmoCreate");
		entity = EntityManager.EnumEntity();
		while (entity != 0)
		{
			if (MainCamera->PixelFromWorldPt(&entityPixel, entity->Position(), ViewportWidth, ViewportHeight))
			{
					pixelDistance = Distance(MousePixel, entityPixel);
					if (pixelDistance < nearestDistance)
					{
						NearestEntity = entity;
						//RenderText(outText.str(), 0, 0, 1.0f, 1.0f, 0.0f);
						nearestDistance = pixelDistance;
					}
			}
			entity = EntityManager.EnumEntity();
		}
		EntityManager.EndEnumEntities();

		if (NearestEntity != NULL)
		{
			if (KeyHeld(Mouse_RButton))
			{
				CVector3 MousePointer = MainCamera->WorldPtFromPixel(MousePixel, ViewportWidth, ViewportHeight);
				CVector3 RayCast = Normalise(MousePointer - MainCamera->Position());
				CVector3 NewPos = MainCamera->Position() + ((-MainCamera->Position().y / RayCast.y) * RayCast);
				//CVector3 CameraPos = MainCamera->Position() + RayCast * 100;
				NearestEntity->Position().x = NewPos.x;
				NearestEntity->Position().y = NewPos.y;
				NearestEntity->Position().z = NewPos.z;
			}
		}
		/* This allows the HealthCreate to be picked up */
		EntityManager.BeginEnumEntities("", "", "HealthCreate");
		entity = EntityManager.EnumEntity();
		while (entity != 0)
		{
			if (MainCamera->PixelFromWorldPt(&entityPixel, entity->Position(), ViewportWidth, ViewportHeight))
			{
				pixelDistance = Distance(MousePixel, entityPixel);
				if (pixelDistance < nearestDistance)
				{
					NearestEntity = entity;
					//RenderText(outText.str(), 0, 0, 1.0f, 1.0f, 0.0f);
					nearestDistance = pixelDistance;
				}
			}
			entity = EntityManager.EnumEntity();
		}
		EntityManager.EndEnumEntities();

		if (NearestEntity != NULL)
		{
			if (KeyHeld(Mouse_RButton))
			{
				CVector3 MousePointer = MainCamera->WorldPtFromPixel(MousePixel, ViewportWidth, ViewportHeight);
				CVector3 RayCast = Normalise(MousePointer - MainCamera->Position());
				CVector3 NewPos = MainCamera->Position() + ((-MainCamera->Position().y / RayCast.y) * RayCast);
				//CVector3 CameraPos = MainCamera->Position() + RayCast * 100;
				NearestEntity->Position().x = NewPos.x;
				NearestEntity->Position().y = NewPos.y;
				NearestEntity->Position().z = NewPos.z;
			}
		}
	}


	// Update the scene between rendering
	void UpdateScene(float updateTime)
	{
		// Call all entity update functions
		EntityManager.UpdateAllEntities(updateTime);

		// Set camera speeds
		// Key F1 used for full screen toggle
		if (KeyHit(Key_F2)) CameraMoveSpeed = 5.0f;
		if (KeyHit(Key_F3)) CameraMoveSpeed = 40.0f;

		// System messages
		// Go

		/* Runs through all the Scenery */
		EntityManager.BeginEnumEntities("", "", "Scenery");
		CEntity* entity = EntityManager.EnumEntity();
		while (entity != 0)
		{
			/* If the entity is a quad it will update the position*/
			if (entity->GetName() == "Quad")
			{
				CEntity* EntityArray[4];
				for (int i = 0; i < 4; i++)
				{
					EntityArray[i] = entity;
					entity = EntityManager.EnumEntity();
				}
				/* Runs through all the tanks so it get set there patrol points to the quads */
				for (int j = 0; j < TankEntities.size(); j++)
				{
					if (TankEntities.at(j)->GetTeam() == 0)
					{
						for (int i = 0; i < TeamOnePatrolList.size(); i++)
						{
							TeamOnePatrolList[i] = EntityArray[i]->Position();
						}
						TankEntities.at(j)->SetPatrolList(TeamOnePatrolList);
					}
				}

			}
			else
			{
				entity = EntityManager.EnumEntity();
			}
		}
		EntityManager.EndEnumEntities();

		/* Runs through all the Scenery */
		EntityManager.BeginEnumEntities("", "", "Scenery");
		entity = EntityManager.EnumEntity();
		while (entity != 0)
		{
			/* If the entity is a quad it will update the position*/
			if (entity->GetName() == "Quad2")
			{
				CEntity* EntityArray[4];
				for (int i = 0; i < 4; i++)
				{
					EntityArray[i] = entity;
					entity = EntityManager.EnumEntity();
				}
				/* Runs through all the tanks so it get set there patrol points to the quads */
				for (int j = 0; j < TankEntities.size(); j++)
				{
					if (TankEntities.at(j)->GetTeam() == 1)
					{
						for (int i = 0; i < TeamTwoPatrolList.size(); i++)
						{
							TeamTwoPatrolList[i] = EntityArray[i]->Position();
						}
						TankEntities.at(j)->SetPatrolList(TeamTwoPatrolList);
					}

				}
			}
			else
			{
				entity = EntityManager.EnumEntity();
			}
		}
		EntityManager.EndEnumEntities();

		/* When 1 is pressed it will send a message to all the tanks to start */
		if (KeyHit(Key_1))
		{
			SMessage msg;
			EntityManager.BeginEnumEntities("", "", "Tank");
			CEntity* entity = EntityManager.EnumEntity();
			while (entity != 0)
			{
				TEntityUID UID = entity->GetUID();
				//TanksUIDs[i] = UID;
				msg.type = Msg_Start;
				msg.from = SystemUID;
				Messenger.SendMessage(UID, msg);
				entity = EntityManager.EnumEntity();
			}
			EntityManager.EndEnumEntities();
		}

		/* This will allow the user to use the chase camera */
		if (TanksUIDs.at(Counter) == TankEntities.at(Counter)->GetUID())
		{
			if (TankEntities.at(Counter)->GetFollowed() == false)
			{
				if (KeyHit(Key_3))
				{
					/*Sets the chase camera to the current tank*/
					TankEntities.at(Counter)->SetFollowed(true);
				}
			}
		}
		else
		{
			/*Increments Counter so it can watch the next tank*/
			Counter++;
		}
		/* This will switch to the next tank */
		if (KeyHit(Key_3) && TankEntities.at(Counter)->GetFollowed() == true)
		{
			/*Resets the tank propteries before switch*/
			TankEntities.at(Counter)->SetFollowed(false);
			++Counter;
			/*If the counter goes above the max then reset*/
			if (Counter == TankEntities.size())
			{
				Counter = 0;
			}
			TankEntities.at(Counter)->SetFollowed(true);
			while (TanksUIDs.at(Counter) != TankEntities.at(Counter)->GetUID())
			{
				Counter++;
			}
		}
		/* This will constantly update the camera so it can be behind the tank */
		if (TankEntities.at(Counter)->GetFollowed() == true && TanksUIDs.at(Counter) == TankEntities.at(Counter)->GetUID())
		{
			MainCamera->Position() = TankEntities.at(Counter)->Position();
			MainCamera->Position().y += 3.0f;
			//MainCamera->Matrix().FaceTarget(TankEntities[Counter]->Position().kZAxis);
		}
		/* This will exit the camera chase */
		if (KeyHit(Key_4))
		{
			TankEntities.at(Counter)->SetFollowed(false);
			Counter = 0;
		}

		/* This will spawn an ammo create after a set amount of time */
		if (AmmoTimer < 0)
		{
			EntityManager.CreateAmmoCreate("AmmoCreate.01", "", CVector3(Random(-20, 20), 10.0f, Random(-20, 20)), { 0,0,0 }, { 0.2,0.2,0.2 });
			AmmoTimer = 20.0f;
		}
		else
		{
			AmmoTimer -= updateTime;
		}
		/* This will spawn an health create after a set amount of time */
		if (HealthTimer < 0)
		{
			EntityManager.CreateHealthCreate("HealthCreate.01", "", CVector3(Random(-20, 20), 10.0f, Random(-20, 20)), { 0,0,0 }, { 0.2,0.2,0.2 });
			HealthTimer = 30.0f;
		}
		else
		{
			HealthTimer -= updateTime;
		}

		// Stop
		/* When 2 is pressed it will send a message to all the tanks telling them to stop */
		if (KeyHit(Key_2))
		{
			SMessage msg;
			EntityManager.BeginEnumEntities("", "", "Tank");
			CEntity* entity = EntityManager.EnumEntity();
			while (entity != 0)
			{
				TEntityUID UID = entity->GetUID();
				//TanksUIDs[i] = UID;
				msg.type = Msg_Stop;
				msg.from = SystemUID;
				Messenger.SendMessage(UID, msg);
				entity = EntityManager.EnumEntity();
			}
			EntityManager.EndEnumEntities();
		}

		// Move the camera
		MainCamera->Control(Key_Up, Key_Down, Key_Left, Key_Right, Key_W, Key_S, Key_A, Key_D,
			CameraMoveSpeed * updateTime, CameraRotSpeed * updateTime);
	}


} // namespace gen