//////////////////////////////////////////////////////////////////////////
// Mapper020  Nintendo Disk System(FDS)                                 //
//////////////////////////////////////////////////////////////////////////
void	Mapper020::Reset()
{
	irq_type = 0;

	irq_enable = irq_repeat = 0;
	irq_counter = irq_latch = 0;
	irq_occur = 0;
	irq_transfer = 0;

	disk_enable = 0xFF;
	sound_enable = 0xFF;

	block_point = 0;
	block_mode = 0;
	RW_start = 0xFF;
	size_file_data = 0;
	file_amount = 0;
	point = 0;
	first_access = 0;

	disk_eject = 0xFF;
	drive_ready = 0;

	disk_side = 0xFF;	// Eject
	disk = disk_w = NULL;

	SetPROM_Bank( 3, DRAM+0x0000, BANKTYPE_DRAM );
	SetPROM_Bank( 4, DRAM+0x2000, BANKTYPE_DRAM );
	SetPROM_Bank( 5, DRAM+0x4000, BANKTYPE_DRAM );
	SetPROM_Bank( 6, DRAM+0x6000, BANKTYPE_DRAM );
	SetPROM_Bank( 7, nes->rom->GetDISKBIOS(), BANKTYPE_ROM );
	SetCRAM_8K_Bank( 0 );

	// �A���Ă����}���I�u���U�[�Y
	if( nes->rom->GetMakerID() == 0x01 && nes->rom->GetGameID() == 0x4b4d4152 ) {
		irq_type = 2;
		nes->SetRenderMethod( NES::PRE_ALL_RENDER );
	}

	// �����N�̖`��
	if( nes->rom->GetMakerID() == 0x01 && nes->rom->GetGameID() == 0x4C4E4B20 ) {
		irq_type = 1;
	}

////	// ���{���@�_�o�o
////	if( nes->rom->GetMakerID() == 0xA4 && nes->rom->GetGameID() == 0x4D494B20 ) {
////		nes->SetRenderMethod( NES::TILE_RENDER );
////	}
	// �K���t�H�[�X
	if( nes->rom->GetMakerID() == 0xB6 && nes->rom->GetGameID() == 0x47414C20 ) {
		irq_type = 1;
		nes->SetRenderMethod( NES::PRE_ALL_RENDER );
	}
	// �t�@�C�A�[�o��
	if( nes->rom->GetMakerID() == 0xB6 && nes->rom->GetGameID() == 0x46424D20 ) {
		irq_type = 1;
		nes->SetRenderMethod( NES::PRE_ALL_RENDER );
	}
	// �R�c�z�b�g�����[
	if( nes->rom->GetMakerID() == 0x01 && nes->rom->GetGameID() == 0x54445245 ) {
		nes->SetRenderMethod( NES::PRE_ALL_RENDER );
	}

	// �p�b�g�p�b�g�S���t
	if( nes->rom->GetMakerID() == 0x99 && nes->rom->GetGameID() == 0x50504720 ) {
		irq_type = 2;
	}

	// �X�[�p�[�{�[�C�A����
	if( nes->rom->GetMakerID() == 0xC1 && nes->rom->GetGameID() == 0x414C4E20 ) {
		irq_type = 1;
//		nes->SetRenderMethod( NES::TILE_RENDER );
	}

	// �n�b�J�[�C���^�[�i�V���i���n�Ƃ�
	if( nes->rom->GetMakerID() == 0x00 && nes->rom->GetGameID() == 0x00000000 ) {
		irq_type = 1;
	}

DEBUGOUT( "MAKER ID=%02X\n", nes->rom->GetMakerID() );
DEBUGOUT( "GAME  ID=%08X\n", nes->rom->GetGameID() );

	nes->apu->SelectExSound( 4 );

//	ExCmdWrite( EXCMDWR_DISKINSERT, 0 );
	// Disk 0, Side A���Z�b�g
	disk = nes->rom->GetPROM()+16+65500*0;
	disk_w = nes->rom->GetDISK()+16+65500*0;

	disk_side = 0;
	disk_eject = 0xFF;
	drive_ready = 0;
	disk_mount_count = 119;

	nes->Command( NES::NESCMD_DISK_THROTTLE_OFF );

	bDiskThrottle = FALSE;
	DiskThrottleTime = 0;
}

BYTE	Mapper020::ExRead( WORD addr )
{
BYTE	data = 0;

	switch( addr ) {
		case	0x4030:	// Disk I/O status
			data = 0x80;
			data |= (irq_occur && !irq_transfer)?0x01:0x00;
			data |= (irq_occur && irq_transfer)?0x02:0x00;
			irq_occur = 0;
			break;
		case	0x4031:	// Disk data read
			if( !RW_mode )
				return	0xFF;

			first_access = 0;

			if( disk ) {
				switch( block_mode ) {
					case BLOCK_VOLUME_LABEL:
						data = disk[block_point];
						if( block_point < SIZE_VOLUME_LABEL ) {
							block_point++;
						} else {
							data = 0;
						}
						return	data;
					case BLOCK_FILE_AMOUNT:
						data = disk[ block_point + point ];
						if( block_point < SIZE_FILE_AMOUNT ) {
							block_point++;
							file_amount = data;
						} else {
							data = 0;
						}
						return	data;
					case BLOCK_FILE_HEADER:
						data = disk[ block_point + point ];
						if( block_point == 13 )
							size_file_data = data;
						else if( block_point == 14 )
							size_file_data |= data<<8;

						if( block_point < SIZE_FILE_HEADER ) {
							block_point++;
						} else {
							data = 0;
						}
						return	data;
					case BLOCK_FILE_DATA:
						data = disk[ block_point + point ];
						if( block_point < size_file_data + 1 ) {
							block_point++;
						} else {
							data = 0;
						}
						return	data;
				}
			} else {
				return	0xFF;
			}
			break;
		case	0x4032:	// Disk status
			data = 0x40;
			data |= disk_eject?0x01:0x00;
			data |= disk_eject?0x04:0x00;
			data |= (!disk_eject && disk_motor_mode && !drive_reset)?0x00:0x02;
			break;
		case	0x4033:	// External connector data/Battery sense
			data = 0x80;
			break;
		default:
			if( addr >= 0x4040 )
				data = nes->apu->ExRead( addr );
			break;
	}
//DEBUGOUT( "RD $%04X D=%02X L=%3d CYC=%d\n", addr&0xFFFF, data&0xFF, nes->GetScanline(), nes->cpu->GetTotalCycles() );

	return	data;
}

void	Mapper020::ExWrite( WORD addr, BYTE data )
{
//DEBUGOUT( "WR $%04X D=%02X L=%3d CYC=%d\n", addr&0xFFFF, data&0xFF, nes->GetScanline(), nes->cpu->GetTotalCycles() );

	switch( addr ) {
		case	0x4020:	// IRQ latch low
			irq_latch = (irq_latch&0xFF00)|data;
			break;
		case	0x4021:	// IRQ latch high
			irq_latch = (irq_latch&0x00FF)|((WORD)data<<8);
			break;
		case	0x4022:	// IRQ control
			irq_repeat = data & 0x01;
			irq_enable = data & 0x02;

			if( irq_enable ) {
				irq_counter = irq_latch;
#if	1
// 0.37�̎��I�t�ɂ��Ă�
				if( irq_latch ) {
					irq_counter = irq_latch;
				} else {
					irq_occur = 0xFF;
				}
#endif
			}
			break;

		case	0x4023: // 2C33 control
			disk_enable = data & 0x01;
			break;

		case	0x4024:	// Data write
			if( RW_mode )
				break;

			if( first_access ) {
				first_access = 0;
				break;
			}


			if( disk ) {
				switch( block_mode ) {
					case BLOCK_VOLUME_LABEL:
						if( block_point < SIZE_VOLUME_LABEL-1 ) {
							disk[ block_point ] = data;
							disk_w[ block_point ] = 0xFF;
							block_point++;
						}
						break;
					case BLOCK_FILE_AMOUNT:
						if( block_point < SIZE_FILE_AMOUNT ) {
							disk[ block_point + point ] = data;
							disk_w[ block_point + point ] = 0xFF;
							block_point++;
						}
						break;
					case BLOCK_FILE_HEADER:
						if( block_point < SIZE_FILE_HEADER ) {
								disk[ block_point + point ] = data;
								disk_w[ block_point + point ] = 0xFF;
								if( block_point == 13 )
									size_file_data = data;
								else if( block_point == 14 )
									size_file_data |= data << 8;
								block_point++;
						}
						break;
					case BLOCK_FILE_DATA:
						if( block_point < size_file_data+1 ) {
							disk[ block_point + point ] = data;
							disk_w[ block_point + point ] = 0xFF;
							block_point++;
						}
						break;
				}
			}
			break;

		case	0x4025:	// Disk I/O control
			// ���荞�ݓ]��
			irq_transfer = data & 0x80;

			if( !RW_start && (data & 0x40) ) {
				block_point = 0;
				switch( block_mode ) {
					case	BLOCK_READY:
						block_mode = BLOCK_VOLUME_LABEL;
						point = 0;
						break;
					case	BLOCK_VOLUME_LABEL:
						block_mode = BLOCK_FILE_AMOUNT;
						point += SIZE_VOLUME_LABEL;
						break;
					case	BLOCK_FILE_AMOUNT:
						block_mode = BLOCK_FILE_HEADER;
						point += SIZE_FILE_AMOUNT;
						break;
					case	BLOCK_FILE_HEADER:
						block_mode = BLOCK_FILE_DATA;
						point += SIZE_FILE_HEADER;
						break;
					case	BLOCK_FILE_DATA:
						block_mode = BLOCK_FILE_HEADER;
						point += size_file_data+1;
						break;
				}

				// �ŏ��̂P��ڂ̏������݂𖳎����邽��
				first_access = 0xFF;
			}

			// �ǂݏ����X�^�[�g
			RW_start = data & 0x40;

			// �ǂݏ������[�h
			RW_mode = data & 0x04;

			// �ǂݏ����̃��Z�b�g
			if( data&0x02 ) {
				point = 0;
				block_point = 0;
				block_mode = BLOCK_READY;
				RW_start = 0xFF;
				drive_reset = 0xFF;
			} else {
				drive_reset = 0;
			}

			// �f�B�X�N���[�^�[�̃R���g���[��
			disk_motor_mode = data & 0x01;

			// Mirror
			if( data&0x08 ) SetVRAM_Mirror( VRAM_HMIRROR );
			else		SetVRAM_Mirror( VRAM_VMIRROR );
			break;

		case	0x4026:	// External connector output/Battery sense
			break;
		default:
			if( addr >= 0x4040 )
				nes->apu->ExWrite( addr, data );
			break;
	}
}

void	Mapper020::WriteLow( WORD addr, BYTE data )
{
	DRAM[addr-0x6000] = data;
}

void	Mapper020::Write( WORD addr, BYTE data )
{
	if( addr < 0xE000 ) {
		DRAM[addr-0x6000] = data;
	}
}

void	Mapper020::Clock( INT cycles )
{
	if( !irq_transfer && irq_type >= 1 ) {
		if( (irq_counter-=cycles) <= 0 ) {
			if( irq_enable ) {
				irq_occur = 0xFF;

				if( irq_repeat ) {
					irq_counter += irq_latch;
				} else {
//					irq_counter += irq_latch;
					irq_latch = 0x7FFFFFFF;
					irq_enable = 0;
				}
				if( irq_type == 2 ) {
					nes->cpu->IRQ_NotPending();
					return;
				}
			}
		}
	}
	if( !irq_transfer && irq_occur && irq_type != 2 ) {
		nes->cpu->IRQ_NotPending();
	}
}

void	Mapper020::HSync( INT scanline )
{
	if( irq_transfer ) {
		nes->cpu->IRQ_NotPending();
		return;
	}

	if( irq_type == 0 ) {
		if( irq_counter <= 0 ) {
			irq_counter -= 114;

			if( irq_enable ) {
				irq_occur = 0xFF;

				if( irq_repeat ) {
					irq_counter += irq_latch;
				} else {
////					irq_counter += irq_latch;
					irq_latch = 0x7FFFFFF;
					irq_enable = 0;
				}
				if( irq_type == 2 ) {
					nes->cpu->IRQ_NotPending();
				}
			}
		} else if( irq_counter > 0 ) {
			irq_counter -= 114;
		}
	}
}

void	Mapper020::VSync()
{
	if( disk && disk_eject ) {
		if( disk_mount_count > 120 ) {
			disk_eject = 0;
		} else {
			disk_mount_count++;
		}
	}

	if( irq_transfer || (disk && disk_mount_count < 120) ) {
		if( DiskThrottleTime > 2 ) {
			bDiskThrottle = TRUE;
		} else {
			bDiskThrottle = FALSE;
			DiskThrottleTime++;
		}
	} else {
		DiskThrottleTime = 0;
		bDiskThrottle = FALSE;
	}
	if( !bDiskThrottle ) {
		nes->Command( NES::NESCMD_DISK_THROTTLE_OFF );
	} else {
		nes->Command( NES::NESCMD_DISK_THROTTLE_ON );
	}
}

BYTE	Mapper020::ExCmdRead( EXCMDRD cmd )
{
BYTE	data = 0x00;

	return	data;
}

void	Mapper020::ExCmdWrite( EXCMDWR cmd, BYTE data )
{
	switch( cmd ) {
		case	EXCMDWR_NONE:
			break;
		case	EXCMDWR_DISKINSERT:
			disk = nes->rom->GetPROM()+16+65500*data;
			disk_w = nes->rom->GetDISK()+16+65500*data;
			disk_side = data;
			disk_eject = 0xFF;
			drive_ready = 0;
			disk_mount_count = 0;
			break;
		case	EXCMDWR_DISKEJECT:
			disk = NULL;	// �Ƃ肠����
			disk_w = NULL;
			disk_side = 0xFF;
			disk_eject = 0xFF;
			drive_ready = 0;
			disk_mount_count = 0;
			break;
	}
}

void	Mapper020::SaveState( LPBYTE p )
{
	p[0] = irq_enable;
	p[1] = irq_repeat;
	p[2] = irq_occur;
	p[3] = irq_transfer;

	*(INT*)&p[4] = irq_counter;
	*(INT*)&p[8] = irq_latch;

	p[12] = disk_enable;
	p[13] = sound_enable;
	p[14] = RW_start;
	p[15] = RW_mode;
	p[16] = disk_motor_mode;
	p[17] = disk_eject;
	p[18] = drive_ready;
	p[19] = drive_reset;

	*(INT*)&p[20] = block_point;
	*(INT*)&p[24] = block_mode;
	*(INT*)&p[28] = size_file_data;
	*(INT*)&p[32] = file_amount;
	*(INT*)&p[36] = point;

	p[40] = first_access;
	p[41] = disk_side;
	p[42] = disk_mount_count;
}

void	Mapper020::LoadState( LPBYTE p )
{
	irq_enable	= p[0];
	irq_repeat	= p[1];
	irq_occur	= p[2];
	irq_transfer	= p[3];

	irq_counter	= *(INT*)&p[4];
	irq_latch	= *(INT*)&p[8];

	disk_enable	= p[12];
	sound_enable	= p[13];
	RW_start	= p[14];
	RW_mode		= p[15];
	disk_motor_mode	= p[16];
	disk_eject	= p[17];
	drive_ready	= p[18];
	drive_reset	= p[19];

	block_point	= *(INT*)&p[20];
	block_mode	= *(INT*)&p[24];
	size_file_data	= *(INT*)&p[28];
	file_amount	= *(INT*)&p[32];
	point		= *(INT*)&p[36];

	first_access	= p[40];
	disk_side	= p[41];
	disk_mount_count= p[42];

	if( disk_side != 0xFF ) {
		disk = nes->rom->GetPROM()+16+65500*disk_side;
		disk_w = nes->rom->GetDISK()+16+65500*disk_side;
	} else {
		disk = NULL;
		disk_w = NULL;
	}

	// DiskBios Setup(�X�e�[�g�ŏ㏑������Ă����)
	SetPROM_Bank( 7, nes->rom->GetDISKBIOS(), BANKTYPE_ROM );
}