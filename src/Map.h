
// Map.h

// Implementation of in-game coloured maps





#pragma once

#include "Defines.h"
#include "ChunkDef.h"
#include "FastRandom.h"




class cClientHandle;
class cWorld;
class cPlayer;
class cMap;





// tolua_begin

/** Encapsulates an in-game world map. */
class cMap
{
public:
	enum class icon
	{
		MAP_ICON_PLAYER             = 0,
		MAP_ICON_GREEN_ARROW        = 1,  ///< Used for item frames.
		MAP_ICON_RED_ARROW          = 2,
		MAP_ICON_BLUE_ARROW         = 3,
		MAP_ICON_WHITE_CROSS        = 4,
		MAP_ICON_RED_POINTER        = 5,
		MAP_ICON_PLAYER_OUTSIDE     = 6,  ///< Player outside of the boundaries of the map.
		MAP_ICON_PLAYER_FAR_OUTSIDE = 7,  ///< Player far outside of the boundaries of the map.
		MAP_ICON_MANSION            = 8,
		MAP_ICON_MONUMENT           = 9,

		MAP_ICON_WHITE_BANNER       = 10,
		MAP_ICON_ORANGE_BANNER      = 11,
		MAP_ICON_MAGENTA_BANNER     = 12,
		MAP_ICON_LIGHT_BLUE_BANNER  = 13,
		MAP_ICON_YELLOW_BANNER      = 14,
		MAP_ICON_LIME_BANNER        = 15,
		MAP_ICON_PINK_BANNER        = 16,
		MAP_ICON_GRAY_BANNER        = 17,
		MAP_ICON_LIGHT_GRAY_BANNER  = 18,
		MAP_ICON_CYAN_BANNER        = 19,
		MAP_ICON_PURPLE_BANNER      = 20,
		MAP_ICON_BLUE_BANNER        = 21,
		MAP_ICON_BROWN_BANNER       = 22,
		MAP_ICON_GREEN_BANNER       = 23,
		MAP_ICON_RED_BANNER         = 24,
		MAP_ICON_BLACK_BANNER       = 25,
		MAP_ICON_TREASURE_MARKER    = 26,
	};

	// tolua_end

	enum class DecoratorType
	{
		PLAYER = 0,
		FRAME,
		BANNER,
		PERSISTENT,
	};

	/** Encapsulates a map decorator.
	A map decorator represents an object drawn on the map that can move freely.
	(e.g. player trackers and item frame pointers)

	Excluding manually placed decorators,
	decorators are automatically managed (allocated and freed) by their parent cMap instance.
	*/
	class Decorator
	{
	public:
		class Key
		{
		public:
			Key(cMap::DecoratorType a_Type, UInt32 a_Id) :
				m_Type(a_Type),
				m_Id(a_Id)
			{
			}

			bool operator < (const cMap::Decorator::Key & a_Rhs) const
			{
				return (m_Type < a_Rhs.m_Type)
				|| ((m_Type == a_Rhs.m_Type) && (m_Id < a_Rhs.m_Id));
			}

			const cMap::DecoratorType m_Type;
			const UInt32 m_Id;
		};

		class Value
		{
		private:
			static char YawToRot(int a_Yaw)
			{
				return CeilC(((a_Yaw - 11.25) * 16) / 360);
			}

		public:

			Value(cMap::icon a_Icon, const Vector3d & a_Position, int a_Yaw, unsigned int a_MapX, unsigned int a_MapZ) :
				m_Icon(a_Icon),
				m_Position(a_Position),
				m_Yaw(a_Yaw),
				m_MapX(a_MapX),
				m_MapZ(a_MapZ),
				m_CurrentRot(YawToRot(a_Yaw)),
				m_Spin(0),
				m_SpinTime(0),
				m_SpinRate(0)
			{
			}

			void Update(cMap::icon a_Icon, const Vector3d & a_Position, int a_Yaw, unsigned int a_MapX, unsigned int a_MapZ, bool a_Spinning)
			{
				m_Icon = a_Icon;
				m_Position = a_Position;
				m_Yaw = a_Yaw;
				m_MapX = a_MapX;
				m_MapZ = a_MapZ;

				if (!a_Spinning)
				{
					m_CurrentRot = YawToRot(a_Yaw);
				}
			}

			void Spin(void)
			{
				if (m_Icon <= cMap::icon::MAP_ICON_BLUE_ARROW)
				{
					if ((m_SpinTime == 0) || (--m_SpinTime == 0))
					{
						m_SpinRate = 1 + GetRandomProvider().RandInt(3);
						m_SpinTime = 8 + GetRandomProvider().RandInt(12) / m_SpinRate;
						m_SpinRate = (GetRandomProvider().RandInt(1) > 0 ? m_SpinRate : -m_SpinRate);
					}
					m_Spin += m_SpinRate;
					m_CurrentRot = m_Spin / 2;
				}
			}

			cMap::icon m_Icon;

			Vector3d m_Position;
			int m_Yaw;

			unsigned int m_MapX;
			unsigned int m_MapZ;

			char m_CurrentRot;

			char m_Spin, m_SpinTime, m_SpinRate;
		};
	};

	static const int MAP_WIDTH = 128;
	static const int MAP_HEIGHT = 128;
	static const unsigned int DEFAULT_TRACKING_DISTANCE = 320;
	static const unsigned int DEFAULT_FAR_TRACKING_DISTANCE = 1026;

	// tolua_begin

	enum eEmptyMapType
	{
		E_EMPTY_MAP_TYPE_LOCATOR = 0, /* Empty maps from older worlds are always locator maps */
		E_EMPTY_MAP_TYPE_SIMPLE,
		E_EMPTY_MAP_TYPE_EXPLORER,
	};

	/** @deprecated These are exported to Lua but are otherwise unused. */
	enum eBaseColor
	{
		E_BASE_COLOR_TRANSPARENT = 0,  /* Air     */
		E_BASE_COLOR_LIGHT_GREEN = 4,  /* Grass   */
		E_BASE_COLOR_LIGHT_BLUE = 5,
		E_BASE_COLOR_LIGHT_BROWN = 8,  /* Sand    */
		E_BASE_COLOR_GRAY_1      = 12, /* Cloth   */
		E_BASE_COLOR_RED         = 16, /* TNT     */
		E_BASE_COLOR_PALE_BLUE   = 20, /* Ice     */
		E_BASE_COLOR_GRAY_2      = 24, /* Iron    */
		E_BASE_COLOR_DARK_GREEN  = 28, /* Foliage */
		E_BASE_COLOR_WHITE       = 32, /* Snow    */
		E_BASE_COLOR_LIGHT_GRAY  = 36, /* Clay    */
		E_BASE_COLOR_BROWN       = 40, /* Dirt    */
		E_BASE_COLOR_DARK_GRAY   = 44, /* Stone   */
		E_BASE_COLOR_BLUE        = 48, /* Water   */
		E_BASE_COLOR_DARK_BROWN  = 52  /* Wood    */
	};

	typedef Byte ColorID;

	// tolua_end

	typedef std::vector<cClientHandle *> cMapClientList;
	typedef std::multimap<Decorator::Key, Decorator::Value> cMapDecoratorList;

	/** Construct an empty map. */
	cMap(unsigned int a_ID, cWorld * a_World);

	/** Construct an empty map at the specified coordinates. */
	cMap(unsigned int a_ID, short a_MapType, int a_CenterX, int a_CenterZ, cWorld * a_World, unsigned int a_Scale = 3);

	/** Construct a filled map as a copy of the given map. */
	cMap(unsigned int a_ID, const cMap & a_Map);

	/** Sends a map update to all registered clients
	Clears the list holding registered clients and decorators */
	void Tick();

	/** Update a circular region with the specified radius and center (in pixels). */
	void UpdateRadius(int a_PixelX, int a_PixelZ, unsigned int a_Radius);

	/** Update a circular region around the specified player. */
	void UpdateRadius(cPlayer & a_Player, unsigned int a_Radius);

	/** Send next update packet to the specified player and remove invalid decorators / clients. */
	void UpdateClient(cPlayer * a_Player);

	// tolua_begin

	void SetPosition(int a_CenterX, int a_CenterZ);

	void SetScale(unsigned int a_Scale) { m_Scale = a_Scale; }

	void SetLocked(bool a_OnOff);

	void SetTrackingPosition(bool a_OnOff);

	void SetUnlimitedTracking(bool a_OnOff);

	void SetTrackingThreshold(unsigned int a_Threshold);

	void SetFarTrackingThreshold(unsigned int a_Threshold);

	bool SetPixel(unsigned int a_X, unsigned int a_Z, ColorID a_Data)
	{
		if ((a_X < MAP_WIDTH) && (a_Z < MAP_HEIGHT))
		{
			auto index = a_Z * MAP_WIDTH + a_X;

			m_Dirty |= (m_Data[index] != a_Data);
			m_Data[index] = a_Data;

			return true;
		}
		else
		{
			return false;
		}
	}

	ColorID GetPixel(unsigned int a_X, unsigned int a_Z) const
	{
		return ((a_X < MAP_WIDTH) && (a_Z < MAP_HEIGHT)) ? m_Data[a_Z * MAP_WIDTH + a_X] : 0;
	}

	unsigned int GetWidth (void) const { return MAP_WIDTH;  }
	unsigned int GetHeight(void) const { return MAP_HEIGHT; }

	unsigned int GetScale(void) const { return m_Scale; }

	int GetCenterX(void) const { return m_CenterX; }
	int GetCenterZ(void) const { return m_CenterZ; }

	unsigned int GetID(void) const { return m_ID; }

	cWorld * GetWorld(void) { return m_World; }

	AString GetName(void) { return m_Name; }

	eDimension GetDimension(void) const;

	unsigned int GetNumPixels(void) const { return MAP_WIDTH * MAP_HEIGHT; }

	unsigned int GetPixelWidth(void) const { return 1 << m_Scale; }

	bool GetLocked(void) const { return m_Locked; }

	bool GetTrackingPosition(void) const { return m_TrackingPosition; }

	bool GetUnlimitedTracking(void) const { return m_UnlimitedTracking; }

	unsigned int GetTrackingThreshold(void) const { return m_TrackingThreshold; }

	unsigned int GetFarTrackingThreshold(void) const { return m_FarTrackingThreshold; }

	unsigned int GetTrackingDistance(const cEntity * a_Entity) const { return GetTrackingDistance(a_Entity->GetPosition()); }

	unsigned int GetTrackingDistance(const Vector3d & a_Position) const
	{
		return std::max(abs(a_Position.x - m_CenterX), abs(a_Position.z - m_CenterZ));
	}

	void AddDecorator(UInt32 a_Id, cMap::icon a_Icon, const Vector3d & a_Position, int a_Yaw)
	{
		AddDecorator(cMap::DecoratorType::PERSISTENT, a_Id, a_Icon, a_Position, a_Yaw);
	}

	void RemoveDecorator(UInt32 a_Id)
	{
		RemoveDecorator(cMap::DecoratorType::PERSISTENT, a_Id);
	}

	// tolua_end

	void AddDecorator(cMap::DecoratorType a_Type, UInt32 a_Id, cMap::icon a_Icon, const Vector3d & a_Position, int a_Yaw);

	void RemoveDecorator(cMap::DecoratorType a_Type, UInt32 a_Id);

	void AddFrame(UInt32 a_Id, const Vector3d & a_Position, int a_Yaw)
	{
		AddDecorator(cMap::DecoratorType::FRAME, a_Id, cMap::icon::MAP_ICON_GREEN_ARROW, a_Position, a_Yaw);
	}

	void RemoveFrame(UInt32 a_Id)
	{
		RemoveDecorator(cMap::DecoratorType::FRAME, a_Id);
	}

	const cMapDecoratorList GetDecorators(void) const { return m_Decorators; }

private:

	/** Update the specified pixel. */
	bool UpdatePixel(unsigned int a_X, unsigned int a_Z);

	unsigned int m_ID;

	/** The zoom level, 2^scale square blocks per pixel */
	unsigned int m_Scale;

	int m_CenterX;
	int m_CenterZ;

	bool m_Dirty;

	bool m_Locked;
	bool m_TrackingPosition;
	bool m_UnlimitedTracking;
	unsigned int m_TrackingThreshold;
	unsigned int m_FarTrackingThreshold;

	/** Column-major array of colours */
	ColorID m_Data[MAP_WIDTH * MAP_HEIGHT];

	cWorld * m_World;

	cMapClientList m_ClientsInCurrentTick;

	cMapDecoratorList m_Decorators;

	AString m_Name;

	friend class cMapManager;
	friend class cMapSerializer;

};  // tolua_export



