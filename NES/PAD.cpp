//////////////////////////////////////////////////////////////////////////
//                                                                      //
//      NES Pad                                                         //
//                                                           Norix      //
//                                               written     2001/02/22 //
//                                               last modify ----/--/-- //
//////////////////////////////////////////////////////////////////////////
#define	WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "typedef.h"
#include "macro.h"

#include "DebugOut.h"
#include "Config.h"

#include "DirectDraw.h"
#include "DirectInput.h"

#include "nes.h"
#include "pad.h"
#include "rom.h"

PAD::PAD( NES* parent ) : nes( parent ) //PAD생성자를 만든 후에 
//PDA가 parent인자를 받아서 nes한테 전달하도록함
{
	excontroller_select = 0;
	expad = NULL;
	bStrobe = FALSE;

	padbit[0] = padbit[1] = padbit[2] = padbit[3] = 0;
	micbit = 0;
}

PAD::~PAD() //~PAD는 virtual 함수이다 
{
	DirectDraw.SetZapperMode( FALSE );
	DirectDraw.SetZapperDrawMode( FALSE );

	DELETEPTR( expad ); // expad를 NULL로 만들어버림 
}

void	PAD::Reset()// PAD의 생성자 RESET
{
	pad1bit = pad2bit = 0;
	bStrobe = FALSE;

	bZapperMode = FALSE;

	ZeroMemory( padcnt, sizeof(padcnt) );

	// Select Extension Devices
	DWORD	crc = nes->rom->GetPROM_CRC();

	if( crc == 0xfbfc6a6c		// Adventures of Bayou Billy, The(E)
	 || crc == 0xcb275051		// Adventures of Bayou Billy, The(U)
	 || crc == 0xfb69c131		// Baby Boomer(Unl)(U)
	 || crc == 0xf2641ad0		// Barker Bill's Trick Shooting(U)
	 || crc == 0xbc1dce96		// Chiller (Unl)(U)
	 || crc == 0x90ca616d		// Duck Hunt(JUE)
	 || crc == 0x59e3343f		// Freedom Force(U)
	 || crc == 0x242a270c		// Gotcha!(U)
	 || crc == 0x7b5bd2de		// Gumshoe(UE)
	 || crc == 0x255b129c		// Gun Sight(J)
	 || crc == 0x8963ae6e		// Hogan's Alley(JU)
	 || crc == 0x51d2112f		// Laser Invasion(U)
	 || crc == 0x0a866c94		// Lone Ranger, The(U)
	 || crc == 0xe4c04eea		// Mad City(J)
	 || crc == 0x9eef47aa		// Mechanized Attack(U)
	 || crc == 0xc2db7551		// Shooting Range(U)
	 || crc == 0x163e86c0		// To The Earth(U)
	 || crc == 0x0d3cf705		// Wild Gunman(J)
	 || crc == 0x389960db ) {	// Wild Gunman(JUE)
		SetExController( EXCONTROLLER_ZAPPER );
	}
	if( crc == 0x35893b67		// Arkanoid(J)
	 || crc == 0x6267fbd1 ) {	// Arkanoid 2(J)
		SetExController( EXCONTROLLER_PADDLE );
	}
	if( crc == 0xff6621ce		// Hyper Olympic(J)
	 || crc == 0xdb9418e8		// Hyper Olympic(Tonosama Ban)(J)
	 || crc == 0xac98cd70 ) {	// Hyper Sports(J)
		SetExController( EXCONTROLLER_HYPERSHOT );
	}
	if( crc == 0xf9def527		// Family BASIC(Ver2.0)
	 || crc == 0xde34526e		// Family BASIC(Ver2.1a)
	 || crc == 0xf050b611		// Family BASIC(Ver3)
	 || crc == 0x3aaeed3f ) {	// Family BASIC(Ver3)(Alt)
		SetExController( EXCONTROLLER_KEYBOARD );
	}
	if( crc == 0xc68363f6 ) {	// Crazy Climber(J)
		SetExController( EXCONTROLLER_CRAZYCLIMBER );
	}
	if( crc == 0x20d22251 ) {	// Top rider(J)
		SetExController( EXCONTROLLER_TOPRIDER );
	}
	if( crc == 0x0cd00488 ) {	// Space Shadow(J)
		SetExController( EXCONTROLLER_SPACESHADOWGUN );
	}
}

void	PAD::SetExController( INT type )
{
	excontroller_select = type;

	DELETEPTR( expad );

	bZapperMode = FALSE;
	DirectDraw.SetZapperMode( FALSE );
	DirectDraw.SetZapperDrawMode( FALSE );

	// ExPad Instance create
	switch( type ) {
		case	EXCONTROLLER_ZAPPER:
			expad = new EXPAD_Zapper( nes ); //expad에 EXPAD_Zapper( nes )를 넣는다
			bZapperMode = TRUE;
			DirectDraw.SetZapperMode( TRUE );
			DirectDraw.SetZapperDrawMode( TRUE );
			break;
		case	EXCONTROLLER_PADDLE:
			expad = new EXPAD_Paddle( nes );
			DirectDraw.SetZapperMode( TRUE );
			break;
		case	EXCONTROLLER_HYPERSHOT:
			expad = new EXPAD_HyperShot( nes );
			break;
		case	EXCONTROLLER_KEYBOARD:
			expad = new EXPAD_Keyboard( nes );
			break;
		case	EXCONTROLLER_CRAZYCLIMBER:
			expad = new EXPAD_CrazyClimber( nes );
			break;
		case	EXCONTROLLER_TOPRIDER:
			expad = new EXPAD_Toprider( nes );
			break;
		case	EXCONTROLLER_SPACESHADOWGUN:
			expad = new EXPAD_SpaceShadowGun( nes );
			bZapperMode = TRUE;
			DirectDraw.SetZapperMode( TRUE );
			DirectDraw.SetZapperDrawMode( TRUE );
			break;
		default:
			break;
	}

	if( expad ) {
		expad->Reset();
	}
}

DWORD	PAD::GetSyncData()
{
DWORD	ret;

	ret = (DWORD)padbit[0]|((DWORD)padbit[1]<<8)|((DWORD)padbit[2]<<16)|((DWORD)padbit[3]<<24);
	ret &= ~0x0C0C0C00;
	ret |= (DWORD)micbit<<8;

	return	ret;
}

void	PAD::SetSyncData( DWORD data )
{
	padbit[0] = (BYTE) data;
	padbit[1] = (BYTE)(data>> 8);
	padbit[2] = (BYTE)(data>>16);
	padbit[3] = (BYTE)(data>>24);
	micbit = (BYTE)((data&0x00000400)>>8);
}

DWORD	PAD::GetSyncExData()
{
DWORD	data = 0;

	switch( excontroller_select ) {
		case	EXCONTROLLER_ZAPPER:
		case	EXCONTROLLER_PADDLE:
		case	EXCONTROLLER_SPACESHADOWGUN:
			{
			LONG	x, y;
				x = expad->GetSyncData( 0 );
				y = expad->GetSyncData( 1 );
				if( x == -1 || y == -1 ) {
					data = 0x80000000;
				} else {
					data = (x&0xFF)|((y&0xFF)<<8);
				}
			}
			if( excontroller_select != EXCONTROLLER_SPACESHADOWGUN ) {
				if( expad->GetSyncData( 2 ) )
					data |= 0x0010000;
			} else {
				data |= (DWORD)expad->GetSyncData( 2 )<<16;
			}
			break;
		case	EXCONTROLLER_CRAZYCLIMBER:
			data = (DWORD)expad->GetSyncData( 0 );
			break;
		case	EXCONTROLLER_TOPRIDER:
			data = (DWORD)expad->GetSyncData( 0 );
			break;
		default:
			break;
	}
	return	data;
}

void	PAD::SetSyncExData( DWORD data )
{
//DEBUGOUT( "PAD::SetSyncExData\n" );
	switch( excontroller_select ) {
		case	EXCONTROLLER_ZAPPER:
		case	EXCONTROLLER_PADDLE:
		case	EXCONTROLLER_SPACESHADOWGUN:
			{
			LONG	x, y;
				if( data & 0x80000000 ) {
					x = -1;
					y = -1;
				} else {
					x = data & 0xFF;
					y = (data&0xFF00)>>8;
				}
				expad->SetSyncData( 0, x );
				expad->SetSyncData( 1, y );
				nes->SetZapperPos( x, y );
				DirectDraw.SetZapperPos( x, y );
			}
			if( excontroller_select != EXCONTROLLER_SPACESHADOWGUN ) {
				if( data & 0x0010000 )
					expad->SetSyncData( 2, 1 );
			} else {
				expad->SetSyncData( 2, (BYTE)(data>>16) );
			}
			break;
		case	EXCONTROLLER_CRAZYCLIMBER:
			expad->SetSyncData( 0, (LONG)data );
			break;
		case	EXCONTROLLER_TOPRIDER:
			expad->SetSyncData( 0, (LONG)data );
			break;
		default:
			break;
	}
}

void	PAD::Sync()
{
	padbit[0] = SyncSub( 0 );
	padbit[1] = SyncSub( 1 );
	padbit[2] = SyncSub( 2 );
	padbit[3] = SyncSub( 3 );

	// ExA, ExB clear
	padbit[1] &= ~0x0C;
	padbit[2] &= ~0x0C;
	padbit[3] &= ~0x0C;

	// Mic
	micbit = 0;
	if( DirectInput.m_Sw[Config.controller.nButton[1][8]]
	 || DirectInput.m_Sw[Config.controller.nButton[1][9]] ) {
		micbit |= 0x04;	// Mic
	}

	// For Excontroller
	if( expad ) {
		expad->Sync();
	}

	// For NSF
	NsfSub();
}

static	INT	ren30fps[] = {
	1, 0
};
static	INT	ren20fps[] = {
	1, 1, 0
};
static	INT	ren15fps[] = {
	1, 1, 0, 0
};
static	INT	ren10fps[] = {
	1, 1, 1, 0, 0, 0
};

static	INT	renmask[] = {
	6, 4, 3, 2,
};
static	INT*	rentbl[] = {
	ren10fps, ren15fps, ren20fps, ren30fps
};

BYTE	PAD::SyncSub( INT no )
{
WORD	bit = 0;

	// Up
	if( DirectInput.m_Sw[Config.controller.nButton[no][0]] || DirectInput.m_Sw[Config.controller.nButton[no][10]] )
		bit |= 1<<4;
	// Down
	if( DirectInput.m_Sw[Config.controller.nButton[no][1]] || DirectInput.m_Sw[Config.controller.nButton[no][11]] )
		bit |= 1<<5;
	// Left
	if( DirectInput.m_Sw[Config.controller.nButton[no][2]] || DirectInput.m_Sw[Config.controller.nButton[no][12]] )
		bit |= 1<<6;
	// Right
	if( DirectInput.m_Sw[Config.controller.nButton[no][3]] || DirectInput.m_Sw[Config.controller.nButton[no][13]] )
		bit |= 1<<7;

	// 同時入力を禁止する
	if( (bit&((1<<4)|(1<<5))) == ((1<<4)|(1<<5)) )
		bit &= ~((1<<4)|(1<<5));
	if( (bit&((1<<6)|(1<<7))) == ((1<<6)|(1<<7)) )
		bit &= ~((1<<6)|(1<<7));

	// A,B
	if( DirectInput.m_Sw[Config.controller.nButton[no][4]] || DirectInput.m_Sw[Config.controller.nButton[no][14]] )
		bit |= 1<<0;
	if( DirectInput.m_Sw[Config.controller.nButton[no][5]] || DirectInput.m_Sw[Config.controller.nButton[no][15]] )
		bit |= 1<<1;

	// A,B Rapid
	if( DirectInput.m_Sw[Config.controller.nButton[no][6]] || DirectInput.m_Sw[Config.controller.nButton[no][16]] )
		bit |= 1<<8;
	if( DirectInput.m_Sw[Config.controller.nButton[no][7]] || DirectInput.m_Sw[Config.controller.nButton[no][17]] )
		bit |= 1<<9;

	// Start, Select, Mic
	if( DirectInput.m_Sw[Config.controller.nButton[no][8]] || DirectInput.m_Sw[Config.controller.nButton[no][18]] )
		bit |= 1<<2;
	if( DirectInput.m_Sw[Config.controller.nButton[no][9]] || DirectInput.m_Sw[Config.controller.nButton[no][19]] )
		bit |= 1<<3;

	// A rapid setup
	if( bit&(1<<8) ) {
		INT	spd = Config.controller.nRapid[no][0];
		if( spd >= 3 ) spd = 3;
		INT*	tbl = rentbl[spd];

		if( padcnt[no][0] >= renmask[spd] )
			padcnt[no][0] = 0;

		if( tbl[padcnt[no][0]] )
			bit |= (1<<0);
		else
			bit &= ~(1<<0);

		padcnt[no][0]++;
	} else {
		padcnt[no][0] = 0;
	}
	// B rapid setup
	if( bit&(1<<9) ) {
		INT	spd = Config.controller.nRapid[no][1];
		if( spd >= 3 ) spd = 3;
		INT*	tbl = rentbl[spd];

		if( padcnt[no][1] >= renmask[spd] )
			padcnt[no][1] = 0;

		if( tbl[padcnt[no][1]] )
			bit |= (1<<1);
		else
			bit &= ~(1<<1);

		padcnt[no][1]++;
	} else {
		padcnt[no][1] = 0;
	}

	return	(BYTE)(bit&0xFF);
}

void	PAD::Strobe()
{
	if( Config.emulator.bFourPlayer ) {
	// NES type
		pad1bit = (DWORD)padbit[0] | ((DWORD)padbit[2]<<8) | 0x00080000;
		pad2bit = (DWORD)padbit[1] | ((DWORD)padbit[3]<<8) | 0x00040000;
	} else {
	// Famicom type
		pad1bit = (DWORD)padbit[0];
		pad2bit = (DWORD)padbit[1];
	}
	pad3bit = (DWORD)padbit[2];
	pad4bit = (DWORD)padbit[3];
}

BYTE	PAD::Read( WORD addr )
{
BYTE	data = 0x00;

	if( addr == 0x4016 ) {
		data = (BYTE)pad1bit&1;
		pad1bit>>=1;
		data |= ((BYTE)pad3bit&1)<<1;
		pad3bit>>=1;
		// Mic
		data |= micbit;

		if( expad ) {
			data |= expad->Read4016();
		}
	}
	if( addr == 0x4017 ) {
		data = (BYTE)pad2bit&1;
		pad2bit>>=1;
		data |= ((BYTE)pad4bit&1)<<1;
		pad4bit>>=1;

		if( expad ) {
			data |= expad->Read4017();
		}
	}

	return	data;
}

void	PAD::Write( WORD addr, BYTE data )
{
	if( addr == 0x4016 ) {
		if( data&0x01 ) {
			bStrobe = TRUE;
		} else if( bStrobe ) {
			bStrobe = FALSE;
			Strobe();
			if( expad ) {
				expad->Strobe();
			}
		}

		if( expad ) {
			expad->Write4016( data );
		}
	}
	if( addr == 0x4017 ) {
		if( expad ) {
			expad->Write4017( data );
		}
	}
}

void	PAD::NsfSub()
{
	nsfbit = 0;

	// Play
	if( DirectInput.m_Sw[Config.controller.nNsfButton[0]] || DirectInput.m_Sw[Config.controller.nNsfButton[10]] )
		nsfbit |= 1<<0;
	// Stop
	if( DirectInput.m_Sw[Config.controller.nNsfButton[1]] || DirectInput.m_Sw[Config.controller.nNsfButton[11]] )
		nsfbit |= 1<<1;

	// Number -1
	if( DirectInput.m_Sw[Config.controller.nNsfButton[2]] || DirectInput.m_Sw[Config.controller.nNsfButton[12]] )
		nsfbit |= 1<<2;
	// Number +1
	if( DirectInput.m_Sw[Config.controller.nNsfButton[3]] || DirectInput.m_Sw[Config.controller.nNsfButton[13]] )
		nsfbit |= 1<<3;
	// Number +16
	if( DirectInput.m_Sw[Config.controller.nNsfButton[4]] || DirectInput.m_Sw[Config.controller.nNsfButton[14]] )
		nsfbit |= 1<<4;
	// Number -16
	if( DirectInput.m_Sw[Config.controller.nNsfButton[5]] || DirectInput.m_Sw[Config.controller.nNsfButton[15]] )
		nsfbit |= 1<<5;

	// 同時入力を禁止する
	if( (nsfbit&((1<<2)|(1<<3))) == ((1<<2)|(1<<3)) )
		nsfbit &= ~((1<<2)|(1<<3));
	if( (nsfbit&((1<<4)|(1<<5))) == ((1<<4)|(1<<5)) )
		nsfbit &= ~((1<<4)|(1<<5));
}

#include "EXPAD_Zapper.cpp"
#include "EXPAD_Paddle.cpp"
#include "EXPAD_HyperShot.cpp"
#include "EXPAD_Keyboard.cpp"
#include "EXPAD_CrazyClimber.cpp"
#include "EXPAD_Toprider.cpp"
#include "EXPAD_SpaceShadowGun.cpp"

