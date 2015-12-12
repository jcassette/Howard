/**
 * @file  rtl2832u.c
 *
 * File originally part of package downloaded from here:
 * 
 * http://www.turnovfree.net/~stybla/linux/v4l-dvb/lv5tdlx/20110614_RTL2832_2836_2840_LINUX+rc.zip
 *
 * Modified by Saul Dickinson <sauldickinson@gmail.com> (September 2011)
 * Modified by Xgaz <xgazza@inwind.it> (October 2011)
 *
 * See end of file for original authors.
 */


#include <linux/module.h>
#include <linux/version.h>

#include "rtl2832u.h"
#include "rtl2832u_io.h"
#include "rtl2832u_ioctl.h"


/*
 * Module parameters
 */
s32 dvb_usb_rtl2832u_debug = 0;
module_param_named(debug,dvb_usb_rtl2832u_debug, int, 0644);
MODULE_PARM_DESC(debug, "Set debugging level (1=info,xfer=2 (or-able))." DVB_USB_DEBUG_STATUS);

s32 demod_default_type = 0;
module_param_named(demod, demod_default_type, int, 0644);
MODULE_PARM_DESC(demod, "Set default demod type(0=dvb-t, 1=dtmb, 2=dvb-c, default=0)"DVB_USB_DEBUG_STATUS);

s32 dtmb_error_packet_discard;
module_param_named(dtmb_err_discard, dtmb_error_packet_discard, int, 0644);
MODULE_PARM_DESC(dtmb_err_discard, "Set error packet discard type(0=not discard, 1=discard)"DVB_USB_DEBUG_STATUS);

s32 dvb_use_rtl2832u_rc_mode = RT_NEC;
module_param_named(rtl2832u_rc_mode, dvb_use_rtl2832u_rc_mode, int, 0644);
MODULE_PARM_DESC(rtl2832u_rc_mode, "Set default rtl2832u_rc_mode(0:rc6 1:rc5  2:nec 3=disable rc, default=2)."DVB_USB_DEBUG_STATUS);

s32 dvb_use_rtl2832u_card_type=1;
module_param_named(rtl2832u_card_type, dvb_use_rtl2832u_card_type, int, 0644);
MODULE_PARM_DESC(rtl2832u_card_type, "Set default rtl2832u_card_type type(0=dongle, 1=mini card , default=1)."DVB_USB_DEBUG_STATUS);




//#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 26)
DVB_DEFINE_MOD_OPT_ADAPTER_NR(adapter_nr);
//#endif


#define RT_RC_POLLING_INTERVAL_TIME_MS   287


// TODO : implement the remote's mouse control mode


static struct rc_map_table rc_map_rtl2832u_table[] = {
	{ 0x40bf, KEY_POWER2 },        // TV POWER
	{ 0x08f7, KEY_POWER },         // PC POWER
	{ 0x58a7, KEY_REWIND },        // REWIND
	{ 0xd827, KEY_PLAY },          // PLAY
	{ 0x22dd, KEY_FASTFORWARD },   // FAST FORWARD
	{ 0x02fd, KEY_STOP },          // STOP
	{ 0x5aa5, KEY_PREVIOUS },      // SKIP BACK
	{ 0x42bd, KEY_PLAYPAUSE },     // PAUSE
	{ 0xa25d, KEY_NEXT },          // SKIP FOWARD
	{ 0x12ed, KEY_RECORD },        // RECORD
	{ 0x28d7, KEY_BACK },          // BACK
	{ 0xa857, KEY_INFO },          // MORE
//	{ 0x28d7, BTN_LEFT },          // MOUSE LEFT BUTTON
//	{ 0xa857, BTN_RIGHT },         // MOUSE RIGHT BUTTON
	{ 0x6897, KEY_UP},             // UP
	{ 0x48b7, KEY_DOWN},           // DOWN
	{ 0xe817, KEY_LEFT },          // LEFT
	{ 0x30cf, KEY_RIGHT },         // RIGHT
	{ 0x18e7, KEY_OK },            // OK 
	{ 0xc23d, KEY_ZOOM },          // ASPECT
//	{ 0xea15, KEY_??? },           // MOUSE
	{ 0x708f, KEY_RED },           // RED 
	{ 0xc837, KEY_GREEN },         // GREEN 
	{ 0x8877, KEY_YELLOW },        // YELLOW
	{ 0x9867, KEY_BLUE },          // BLUE
	{ 0x807f, KEY_VOLUMEUP },      // VOL UP 
	{ 0x7887, KEY_VOLUMEDOWN },    // VOL DOWN 
	{ 0xb04f, KEY_HOME },          // HOME
	{ 0x00ff, KEY_MUTE },          // MUTE 
	{ 0xd22d, KEY_CHANNELUP },     // CH UP 
	{ 0xf20d, KEY_CHANNELDOWN },   // CH DOWN 
	{ 0x50af, KEY_0 },             // 0 
	{ 0xf807, KEY_1 },             // 1 
	{ 0xc03f, KEY_2 },             // 2 
	{ 0x20df, KEY_3 },             // 3 
	{ 0xa05f, KEY_4 },             // 4 
	{ 0x38c7, KEY_5 },             // 5 
	{ 0x609f, KEY_6 },             // 6 
	{ 0xe01f, KEY_7 },             // 7 
	{ 0x10ef, KEY_8 },             // 8 
	{ 0xb847, KEY_9 },             // 9
	{ 0x906f, KEY_NUMERIC_STAR },  // *
	{ 0xd02f, KEY_NUMERIC_POUND }, // #
	{ 0x52ad, KEY_EPG },           // GUIDE
	{ 0x926d, KEY_VIDEO },         // RTV
	{ 0x32cd, KEY_HELP },          // HELP
	{ 0xca35, KEY_CYCLEWINDOWS },  // PIP(?)
	{ 0xb24d, KEY_RADIO },         // RADIO
	{ 0x0af5, KEY_DVD },           // DVD
	{ 0x8a75, KEY_AUDIO },         // AUDIO
	{ 0x4ab5, KEY_TITLE }          // TITLE
};


enum rc_status_define{
	RC_FUNCTION_SUCCESS =0,
	RC_FUNCTION_UNSUCCESS
};


static s32 rtl2832u_remote_control_state = 0;


static s32 SampleNum2TNum[] = 
{
	0,0,0,0,0,				
	1,1,1,1,1,1,1,1,1,			
	2,2,2,2,2,2,2,2,2,			
	3,3,3,3,3,3,3,3,3,			
	4,4,4,4,4,4,4,4,4,			
	5,5,5,5,5,5,5,5,			
	6,6,6,6,6,6,6,6,6,			
	7,7,7,7,7,7,7,7,7,			
	8,8,8,8,8,8,8,8,8,			
	9,9,9,9,9,9,9,9,9,			
	10,10,10,10,10,10,10,10,10,	
	11,11,11,11,11,11,11,11,11,	
	12,12,12,12,12,12,12,12,12,	
	13,13,13,13,13,13,13,13,13,	
	14,14,14,14,14,14,14		
};


// IR RC register table 
static const RT_rc_set_reg_struct rtl2832u_rc_initial_table[]= 
{
	{ RTD2832U_SYS, RC_USE_DEMOD_CTL1,      0x00, OP_AND, 0xfb },
	{ RTD2832U_SYS, RC_USE_DEMOD_CTL1,      0x00, OP_AND, 0xf7 },
	{ RTD2832U_USB, USB_CTRL,               0x00, OP_OR,  0x20 },
	{ RTD2832U_SYS, SYS_GPD,                0x00, OP_AND, 0xf7 },
	{ RTD2832U_SYS, SYS_GPOE,               0x00, OP_OR,  0x08 },
	{ RTD2832U_SYS, SYS_GPO,                0x00, OP_OR,  0x08 },
	{ RTD2832U_RC,  IR_RX_CTRL,             0x20, OP_NO,  0xff },
	{ RTD2832U_RC,  IR_RX_BUFFER_CTRL,      0x80, OP_NO,  0xff },
	{ RTD2832U_RC,  IR_RX_IF,               0xff, OP_NO,  0xff },
	{ RTD2832U_RC,  IR_RX_IE,               0xff, OP_NO,  0xff },
	{ RTD2832U_RC,  IR_MAX_DURATION0,       0xd0, OP_NO,  0xff },
	{ RTD2832U_RC,  IR_MAX_DURATION1,       0x07, OP_NO,  0xff },
	{ RTD2832U_RC,  IR_IDLE_LEN0,           0xc0, OP_NO,  0xff },
	{ RTD2832U_RC,  IR_IDLE_LEN1,           0x00, OP_NO,  0xff },
	{ RTD2832U_RC,  IR_GLITCH_LEN,          0x03, OP_NO,  0xff },
	{ RTD2832U_RC,  IR_RX_CLK,              0x09, OP_NO,  0xff },
	{ RTD2832U_RC,  IR_RX_CONFIG,           0x1c, OP_NO,  0xff },
	{ RTD2832U_RC,  IR_MAX_H_Tolerance_LEN, 0x1e, OP_NO,  0xff },
	{ RTD2832U_RC,  IR_MAX_L_Tolerance_LEN, 0x1e, OP_NO,  0xff },
	{ RTD2832U_RC,  IR_RX_CTRL,             0x80, OP_NO,  0xff }
};


s32 rtl2832u_init_rc(struct dvb_usb_device* usbdev)
{ 
	//begin setting
	s32 ret = RC_FUNCTION_SUCCESS;
	u8 data = 0, i = 0, RCInitialTableSize = 0;

	deb_rc("+rc_%s\n", __FUNCTION__);

	RCInitialTableSize = sizeof(rtl2832u_rc_initial_table)/sizeof(RT_rc_set_reg_struct);

	for(i = 0; i < RCInitialTableSize; i++)
	{	
		switch(rtl2832u_rc_initial_table[i].type)
		{
			case RTD2832U_SYS:
			case RTD2832U_USB:
				data = rtl2832u_rc_initial_table[i].data;
				if(rtl2832u_rc_initial_table[i].op != OP_NO)
				{
					if(read_usb_sys_char_bytes(usbdev,
					                           rtl2832u_rc_initial_table[i].type,
					                           rtl2832u_rc_initial_table[i].address,
					                           &data,
					                           LEN_1))
					{
						deb_rc("+%s : rc- usb or sys register read error! \n", __FUNCTION__);
						ret=RC_FUNCTION_UNSUCCESS;
						goto error;
					}					
				
					if(rtl2832u_rc_initial_table[i].op == OP_AND) {
						data &=  rtl2832u_rc_initial_table[i].op_mask;	
					}
					else{//OP_OR
						data |= rtl2832u_rc_initial_table[i].op_mask;
					}			
				}
				
				if(write_usb_sys_char_bytes(usbdev,
				                            rtl2832u_rc_initial_table[i].type,
				                            rtl2832u_rc_initial_table[i].address,
				                            &data,
				                            LEN_1))
				{
						deb_rc("+%s : rc- usb or sys register write error! \n", __FUNCTION__);
						ret= RC_FUNCTION_UNSUCCESS;
						goto error;
				}
				break;

			case RTD2832U_RC:
				data = rtl2832u_rc_initial_table[i].data;
				if(rtl2832u_rc_initial_table[i].op != OP_NO)
				{
					if(read_rc_char_bytes(usbdev,
					                      rtl2832u_rc_initial_table[i].type,
					                      rtl2832u_rc_initial_table[i].address,
					                      &data,
					                      LEN_1))
					{
						deb_rc("+%s : rc -ir register read error! \n", __FUNCTION__);
						ret=RC_FUNCTION_UNSUCCESS;
						goto error;
					}					
				
					if(rtl2832u_rc_initial_table[i].op == OP_AND){
						data &=  rtl2832u_rc_initial_table[i].op_mask;	
					}
					else{//OP_OR
					    data |=  rtl2832u_rc_initial_table[i].op_mask;
					}			
				}

				if(write_rc_char_bytes(usbdev,
				                       rtl2832u_rc_initial_table[i].type,
				                       rtl2832u_rc_initial_table[i].address,
				                       &data,
				                       LEN_1))
				{
					deb_rc("+%s : rc -ir register write error! \n", __FUNCTION__);
					ret=RC_FUNCTION_UNSUCCESS;
					goto error;
				}
				break;

			default:
				deb_rc("+%s : rc table error! \n", __FUNCTION__);
				ret=RC_FUNCTION_UNSUCCESS;
				goto error;			     	
				break;	
		}	
	}

	rtl2832u_remote_control_state = RC_INSTALL_OK;
	ret = RC_FUNCTION_SUCCESS;

error: 
	deb_rc("-rc_%s ret = %d \n", __FUNCTION__, ret);
	return ret;
}


static s32 rc_RC6_RX(u8* rt_uccode,u8 byte_num,u8 *uccode)
{
	u8  *code = rt_uccode;
	s32 tNum = 0;
	u8  ucBits[RC6_bits_num];
	u8  i = 0;
	u8  state = WAITING_6T;
	s32 lastTNum = 0;
	s32 currentBit = 0;
	s32 ret = RC_FUNCTION_SUCCESS;
	u8  highestBit = 0;
	u8  lowBits = 0;
	u32 scancode = 0;
	
	if(byte_num < RC6_para1)
	{
		deb_rc("Bad rt uc code received, byte_num is error\n");
		ret= RC_FUNCTION_UNSUCCESS;
		goto error;
	}

	while(byte_num > 0)
	{
		highestBit = (*code)&0x80;
		lowBits = (*code) & 0x7f;
		tNum=SampleNum2TNum[lowBits];
		
		if(highestBit != 0)	tNum = -tNum;

		code++;
		byte_num--;

		if(tNum <= -6)	 state = WAITING_6T;

		if(WAITING_6T == state)
		{
			if(tNum <= -6)	state = WAITING_2T_AFTER_6T;
		}
		else if(WAITING_2T_AFTER_6T == state)
		{
			if(2 == tNum)	
			{
				state = WAITING_NORMAL_BITS;
				lastTNum   = 0;
				currentBit = 0;
			}
			else 	state = WAITING_6T;
		} 
		else if(WAITING_NORMAL_BITS == state)
		{
			if(0 == lastTNum)	lastTNum = tNum;
			else	{
				if(lastTNum < 0)	ucBits[currentBit]=1;
				else			ucBits[currentBit]=0;

				currentBit++;

				if(currentBit >= RC6_bits_num)	{
 					deb_rc("Bad frame received, bits num is error\n");
					currentBit = RC6_bits_num -1 ;

				}

				if(tNum > 3){
					for(i=0;i<RC6_para2;i++){
						if(ucBits[i+RC6_para4])
							scancode |= (0x01 << (RC6_para2 - i - 1));
					}	
				}
				else{
					lastTNum += tNum;	
				}							
			}			
		}	
	}

	uccode[0]=(u8)((scancode>>24) & RC6_BITS_mask0);
	uccode[1]=(u8)((scancode>>16) & RC6_BITS_mask1);
	uccode[2]=(u8)((scancode>>8)  & RC6_BITS_mask2);
	uccode[3]=(u8)((scancode>>0)  & RC6_BITS_mask3);
	
	deb_rc("-rc_%s 3::rc6:%x %x %x %x \n", __FUNCTION__, uccode[0], uccode[1], uccode[2], uccode[3]);
	ret = RC_FUNCTION_SUCCESS;

error:
	return ret;
}


static s32 rc_RC5_RX(u8* rt_uccode,u8 byte_num,u8* uccode)
{
	u8 *code = rt_uccode;
	u8  ucBits[RC5_bits_num];
	u8 i=0,currentBit=0,index=0;
	u32 scancode=0;
	s32 ret= RC_FUNCTION_SUCCESS;

	deb_rc("+rc_%s \n", __FUNCTION__);

	if(byte_num < RC5_para1)
	{
		deb_rc("Bad rt uc code received, byte_num = %d is error\n",byte_num);
		ret = RC_FUNCTION_UNSUCCESS;
		goto error;
	}
	
	memset(ucBits, 0, RC5_bits_num);		

	for(i = 0; i < byte_num; i++)	{
		if((code[i] & RC5_para2) < RC5_para3)
			index = RC5_para5;
		else
			index = RC5_para6;

		ucBits[i]= (code[i] & 0x80) + index;
	}

	if(ucBits[0] != RC5_para_uc_1 && ucBits[0] != RC5_para_uc_2) {
		ret = RC_FUNCTION_UNSUCCESS;
		goto error;
	}

	if(ucBits[1] != RC5_para5 && ucBits[1] != RC5_para6) {
		ret = RC_FUNCTION_UNSUCCESS;
		goto error;
	}

	if(ucBits[2] >= RC5_para_uc_1)
		ucBits[2] -= 0x01;
	else {
		ret= RC_FUNCTION_UNSUCCESS;
		goto error;
	}
	
	i = 0x02;
	currentBit = 0x00;

	while(i < byte_num-1)
	{
		if(currentBit >= 32)
			break;

		if((ucBits[i] & 0x0f) == 0x0)	{
			i++;
			continue;
		}

		if(ucBits[i++] == 0x81) {
			if(ucBits[i] >=0x01)	{
				scancode |= 0x00 << (31 - currentBit++);
			}	 							
		}
		else {
			if(ucBits[i] >=0x81)	{
				scancode |= 0x01 << (31 - currentBit++); 
			}
		}
				
		ucBits[i] -= 0x01;
		continue;
	}

	uccode[3] = (u8)((scancode>>16) & RC5_bits_mask3);
	uccode[2] = (u8)((scancode>>24) & RC5_bits_mask2);
	uccode[1] = (u8)((scancode>>8)  & RC5_bits_mask1);
	uccode[0] = (u8)((scancode>>0)  & RC5_bits_mask0);

	deb_rc("-rc_%s rc5:%x %x %x %x -->scancode =%x\n", __FUNCTION__,
	       uccode[0], uccode[1], uccode[2], uccode[3], scancode);

	ret = RC_FUNCTION_SUCCESS;

error:
	return ret;
}


static s32 rc_NEC_RX(u8* rt_uccode, u8 byte_num, u8* uccode)
{
	u8* code = rt_uccode;
	u8  i = 0;
	u32 scancode = 0;
	u8  out_io = 0;
			
	s32 ret = RC_FUNCTION_SUCCESS;

	deb_rc("+%s\n", __FUNCTION__);

	if(byte_num < NEC_para1)
		goto error;

	if(code[0] != NEC_para2)
		goto error;

	if((code[1] < NEC_para3) || (code[1] > NEC_para4))
		goto error;	

	if((code[2] < NEC_para5) && (code[2] >NEC_para6))
	{ 
		if((code[3] < NEC_para7) && (code[3] > NEC_para8) && (code[4] == NEC_para9))
			scancode=0xffff;
		else
			goto error;
	}
	else if((code[2] < NEC_para10) && (code[2] > NEC_para11)) 
	{
	 	for(i = 3; i < 68; i++)
		{  
			if((i % 2) == 1)
			{
				if((code[i] > NEC_para7) || (code[i] < NEC_para8))
				{ 
					deb_rc("Bad rt uc code received[4]\n");
					ret= RC_FUNCTION_UNSUCCESS;
					goto error;
				}			
			}
			else
			{
				if(code[i] < NEC_para12)
					out_io = 0;
				else
					out_io=1;

				scancode |= (out_io << (31 -(i-4)/2));
			}
		} 
	}
	else
	  	goto error;

	uccode[0] = (u8)((scancode >> 24) & NEC_bits_mask0);
	uccode[1] = (u8)((scancode >> 16) & NEC_bits_mask1);
	uccode[2] = (u8)((scancode >>  8) & NEC_bits_mask2);
	uccode[3] = (u8)((scancode >>  0) & NEC_bits_mask3);
	ret = RC_FUNCTION_SUCCESS;

error:
	if(ret != RC_FUNCTION_SUCCESS)
		deb_rc("-%s (ERROR)\n", __FUNCTION__);
	else
		deb_rc("-%s NEC : %x\n", __FUNCTION__, scancode);

	return ret;
}


static s32 rtl2832u_rc_query(struct dvb_usb_device* usbdev, u32 *event, s32 *state)
{
	static const RT_rc_set_reg_struct flush_table1[] = {
		{ RTD2832U_RC, IR_RX_CTRL,        0x20, OP_NO, 0xff },
		{ RTD2832U_RC, IR_RX_BUFFER_CTRL, 0x80, OP_NO, 0xff },
		{ RTD2832U_RC, IR_RX_IF,          0xff, OP_NO, 0xff },
		{ RTD2832U_RC, IR_RX_IE,          0xff, OP_NO, 0xff },        
		{ RTD2832U_RC, IR_RX_CTRL,        0x80, OP_NO, 0xff } 
	};

	static const RT_rc_set_reg_struct flush_table2[] = {
		{ RTD2832U_RC, IR_RX_IF,          0x03, OP_NO, 0xff },
		{ RTD2832U_RC, IR_RX_BUFFER_CTRL, 0x80, OP_NO, 0xff },	
		{ RTD2832U_RC, IR_RX_CTRL,        0x80, OP_NO, 0xff } 
	};

	u8  data = 0, i = 0, byte_count = 0;
	s32 ret = 0;
	u8  rt_u8_code[rt_code_len];
	u8  ucode[4];
	u16 scancode = 0;
	u32 tableSize = 0;

	deb_rc("+%s\n", __FUNCTION__);
	
	if((dvb_use_rtl2832u_rc_mode >= MAX_RC_PROTOCOL_NUM) || (dvb_use_rtl2832u_rc_mode < 0))	
	{
		deb_rc("%s : invalid RC mode (%d)", __FUNCTION__, dvb_use_rtl2832u_rc_mode);
		return 0;
	}

	if(rtl2832u_remote_control_state == RC_NO_SETTING)
		ret = rtl2832u_init_rc(usbdev);	

	if(read_rc_char_bytes(usbdev ,RTD2832U_RC, IR_RX_IF,&data ,LEN_1)) 
	{
		ret = -1;
		goto error;
	}	

	if(!(data & receiveMaskFlag1))
	{
		ret = 0;
		goto error;
	}	
	
	if(data & receiveMaskFlag2)
	{
		if(read_rc_char_bytes(usbdev, RTD2832U_RC, IR_RX_BC, &byte_count, LEN_1))
		{
			//deb_rc("%s : rc -ir register read error! \n", __FUNCTION__);
			ret=-1;
			goto error;
		}

		if(byte_count == 0)
			goto error;
				
		if((byte_count % LEN_2) == 1)
		   byte_count += LEN_1;

		if(byte_count > rt_code_len)
			byte_count = rt_code_len;	
					
		memset(rt_u8_code, 0, rt_code_len);
			
		if(read_rc_char_bytes(usbdev, RTD2832U_RC, IR_RX_BUF, rt_u8_code ,0x80)) 
		{
			//deb_rc("%s : rc -ir register read error! \n", __FUNCTION__);
			ret=-1;
			goto error;
		}
				
		memset(ucode,0,4);
		
		ret=0;

		deb_rc("%s : rt_u8_code = 0x%x, byte_count = 0x%x, ucode = 0x%x\n", __FUNCTION__, rt_u8_code[0], byte_count, ucode[0]);

		if(dvb_use_rtl2832u_rc_mode == RT_RC6)
			ret = rc_RC6_RX(rt_u8_code, byte_count, ucode);
		else if(dvb_use_rtl2832u_rc_mode == RT_RC5)
			ret = rc_RC5_RX(rt_u8_code, byte_count, ucode);
		else if(dvb_use_rtl2832u_rc_mode == RT_NEC)
			ret = rc_NEC_RX(rt_u8_code, byte_count, ucode);
		else  
		{
			deb_rc("%s : rc - unknow rc protocol set ! \n", __FUNCTION__);
			ret=-1;
			goto error;	
		}

		if((ret != RC_FUNCTION_SUCCESS) || (ucode[0] ==0 && ucode[1] ==0 && ucode[2] ==0 && ucode[3] ==0))   
		{
			deb_rc("%s : rc-rc is error scan code ! %x %x %x %x \n", __FUNCTION__,ucode[0],ucode[1],ucode[2],ucode[3]);
			ret=-1;
			goto error;	
		}

		scancode = (ucode[2]<<8) | ucode[3];

		deb_rc("%s : scan code %x %x %x %x, (0x%x) -- len = %d\n", __FUNCTION__,ucode[0],ucode[1],ucode[2],ucode[3], scancode, byte_count);

		tableSize = ARRAY_SIZE(rc_map_rtl2832u_table);

		// Perform lookup
		for(i = 0; i < tableSize; i++)
		{
			if(rc_map_rtl2832u_table[i].scancode == scancode)
			{
				*event = rc_map_rtl2832u_table[i].keycode;
				*state = REMOTE_KEY_PRESSED;
				deb_rc("%s : map number = %d \n", __FUNCTION__, i);	
				break;
			}		
		}

		if(i == tableSize)
			deb_rc("%s : scancode 0x%x NOT found!\n", __FUNCTION__, scancode);

		memset(rt_u8_code,0,rt_code_len);
		byte_count = 0;

		for(i = 0; i < 3; i++)
		{
			data = flush_table2[i].data;

			if(write_rc_char_bytes(usbdev, RTD2832U_RC, flush_table2[i].address, &data, LEN_1))
			{
				deb_rc("%s : rc -ir register write error! \n", __FUNCTION__);
				ret=-1;
				goto error;
			}		
		}

		ret =0;
		return ret;
	}

error:

	memset(rt_u8_code,0,rt_code_len);

	byte_count=0;
	for (i=0;i<flush_step_Number;i++)
	{
		data= flush_table1[i].data;

		if(write_rc_char_bytes(usbdev, RTD2832U_RC, flush_table1[i].address,&data,LEN_1))
		{
			deb_rc("%s : rc -ir register write error! \n", __FUNCTION__);
			ret=-1;
			break;
		}		
	}			   			

	ret = 0;    //must return 0
	deb_rc("-%s\n", __FUNCTION__);
	return ret;
}


static s32 rtl2832u_streaming_ctrl(struct dvb_usb_adapter *adap, s32 onoff)
{
	u8 data[2];

	//3 to avoid  scanning  channels loss
	if(onoff)
	{
		data[0] = data[1] = 0;

		if(write_usb_sys_char_bytes(adap->dev, RTD2832U_USB, USB_EPA_CTL, data, 2))
			goto error;				
	}
	else
	{
		data[0] = 0x10;	//3stall epa, set bit 4 to 1
		data[1] = 0x02;	//3reset epa, set bit 9 to 1

		if(write_usb_sys_char_bytes(adap->dev, RTD2832U_USB, USB_EPA_CTL, data, 2))
			goto error;
	}

	return 0;

error: 
	return -1;
}


static s32 rtl2832u_frontend_attach(struct dvb_usb_adapter* adap)
{
	adap->fe = rtl2832u_fe_attach(adap->dev); 
	return 0;
}


static void rtl2832u_usb_disconnect(struct usb_interface *intf)
{
	try_module_get(THIS_MODULE);
	dvb_usb_device_exit(intf);	
}


static struct dvb_usb_device_properties rtl2832u_1st_properties;
static struct dvb_usb_device_properties rtl2832u_2nd_properties;
static struct dvb_usb_device_properties rtl2832u_3th_properties;
static struct dvb_usb_device_properties rtl2832u_4th_properties;
static struct dvb_usb_device_properties rtl2832u_5th_properties;
static struct dvb_usb_device_properties rtl2832u_6th_properties;
static struct dvb_usb_device_properties rtl2832u_7th_properties;
static struct dvb_usb_device_properties rtl2832u_8th_properties;
static struct dvb_usb_device_properties rtl2832u_9th_properties;


//#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 26)
static s32 rtl2832u_usb_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	if (!intf->altsetting->desc.bNumEndpoints)
	        return -ENODEV;
			
	if((0 == dvb_usb_device_init(intf, &rtl2832u_1st_properties, THIS_MODULE, NULL, adapter_nr)) ||
	   (0 == dvb_usb_device_init(intf, &rtl2832u_2nd_properties, THIS_MODULE, NULL, adapter_nr)) ||
	   (0 == dvb_usb_device_init(intf, &rtl2832u_3th_properties, THIS_MODULE, NULL, adapter_nr)) ||
	   (0 == dvb_usb_device_init(intf, &rtl2832u_4th_properties, THIS_MODULE, NULL, adapter_nr)) ||
	   (0 == dvb_usb_device_init(intf, &rtl2832u_5th_properties, THIS_MODULE, NULL, adapter_nr)) ||
	   (0 == dvb_usb_device_init(intf, &rtl2832u_6th_properties, THIS_MODULE, NULL, adapter_nr)) ||
	   (0 == dvb_usb_device_init(intf, &rtl2832u_7th_properties, THIS_MODULE, NULL, adapter_nr)) ||
	   (0 == dvb_usb_device_init(intf, &rtl2832u_8th_properties, THIS_MODULE, NULL, adapter_nr)) ||
	   (0 == dvb_usb_device_init(intf, &rtl2832u_9th_properties, THIS_MODULE, NULL, adapter_nr)))
	{
		return 0;
	}

	return -ENODEV;
}


static struct usb_device_id rtl2832u_usb_table [] = {								
	{ USB_DEVICE(USB_VID_REALTEK, USB_PID_RTL2832_WARM) },       // 0			
	{ USB_DEVICE(USB_VID_REALTEK, USB_PID_RTL2838_WARM) },       // 1
	{ USB_DEVICE(USB_VID_REALTEK, USB_PID_RTL2836_WARM) },       // 2
	{ USB_DEVICE(USB_VID_REALTEK, USB_PID_RTL2839_WARM) },       // 3
	{ USB_DEVICE(USB_VID_REALTEK, USB_PID_RTL2840_WARM) },       // 4
	{ USB_DEVICE(USB_VID_REALTEK, USB_PID_RTL2841_WARM) },       // 5
	{ USB_DEVICE(USB_VID_REALTEK, USB_PID_RTL2834_WARM) },       // 6
	{ USB_DEVICE(USB_VID_REALTEK, USB_PID_RTL2837_WARM) },       // 7
	{ USB_DEVICE(USB_VID_REALTEK, USB_PID_RTL2820_WARM) },       // 8
	{ USB_DEVICE(USB_VID_REALTEK, USB_PID_RTL2821_WARM) },       // 9
	{ USB_DEVICE(USB_VID_REALTEK, USB_PID_RTL2822_WARM) },       // 10
	{ USB_DEVICE(USB_VID_REALTEK, USB_PID_RTL2823_WARM) },       // 11
	{ USB_DEVICE(USB_VID_REALTEK, USB_PID_RTL2810_WARM) },       // 12
	{ USB_DEVICE(USB_VID_REALTEK, USB_PID_RTL2811_WARM) },       // 13
	{ USB_DEVICE(USB_VID_REALTEK, USB_PID_RTL2824_WARM) },       // 14
	{ USB_DEVICE(USB_VID_REALTEK, USB_PID_RTL2825_WARM) },       // 15

	{ USB_DEVICE(USB_VID_DEXATEK, USB_PID_DEXATEK_1101) },       // 16	
	{ USB_DEVICE(USB_VID_DEXATEK, USB_PID_DEXATEK_1102) },       // 17
	{ USB_DEVICE(USB_VID_DEXATEK, USB_PID_DEXATEK_1103) },       // 18	
	{ USB_DEVICE(USB_VID_DEXATEK, USB_PID_DEXATEK_1104) },       // 19
	{ USB_DEVICE(USB_VID_DEXATEK, USB_PID_DEXATEK_1105) },       // 20	
	{ USB_DEVICE(USB_VID_DEXATEK, USB_PID_DEXATEK_1106) },       // 21
	{ USB_DEVICE(USB_VID_DEXATEK, USB_PID_DEXATEK_1107) },       // 22	
	{ USB_DEVICE(USB_VID_DEXATEK, USB_PID_DEXATEK_1108) },       // 23
	{ USB_DEVICE(USB_VID_DEXATEK, USB_PID_DEXATEK_2101) },       // 24	
	{ USB_DEVICE(USB_VID_DEXATEK, USB_PID_DEXATEK_8202) },       // 25
	{ USB_DEVICE(USB_VID_DEXATEK, USB_PID_DEXATEK_9201) },       // 26	
	{ USB_DEVICE(USB_VID_DEXATEK, USB_PID_DEXATEK_3103) },       // 27
	{ USB_DEVICE(USB_VID_DEXATEK, USB_PID_DEXATEK_9202) },       // 28

	{ USB_DEVICE(USB_VID_TERRATEC, USB_PID_TERRATEC_00A9)},      // 29
	{ USB_DEVICE(USB_VID_TERRATEC, USB_PID_TERRATEC_00B3)},      // 30

	{ USB_DEVICE(USB_VID_AZUREWAVE_2, USB_PID_AZUREWAVE_3234) }, // 31
	{ USB_DEVICE(USB_VID_AZUREWAVE_2, USB_PID_AZUREWAVE_3274) }, // 32
	{ USB_DEVICE(USB_VID_AZUREWAVE_2, USB_PID_AZUREWAVE_3282) }, // 33

	{ USB_DEVICE(USB_VID_THP, USB_PID_THP_5013)},                // 34
	{ USB_DEVICE(USB_VID_THP, USB_PID_THP_5020)},                // 35
	{ USB_DEVICE(USB_VID_THP, USB_PID_THP_5026)},                // 36

	{ USB_DEVICE(USB_VID_KWORLD_1ST, USB_PID_KWORLD_D393) },     // 37
	{ USB_DEVICE(USB_VID_KWORLD_1ST, USB_PID_KWORLD_D394) },     // 38
	{ USB_DEVICE(USB_VID_KWORLD_1ST, USB_PID_KWORLD_D395) },     // 39
	{ USB_DEVICE(USB_VID_KWORLD_1ST, USB_PID_KWORLD_D396) },     // 40
	{ USB_DEVICE(USB_VID_KWORLD_1ST, USB_PID_KWORLD_D397) },     // 41
	{ USB_DEVICE(USB_VID_KWORLD_1ST, USB_PID_KWORLD_D398) },     // 42
	{ USB_DEVICE(USB_VID_KWORLD_1ST, USB_PID_KWORLD_D39A) },     // 43
	{ USB_DEVICE(USB_VID_KWORLD_1ST, USB_PID_KWORLD_D39B) },     // 44
	{ USB_DEVICE(USB_VID_KWORLD_1ST, USB_PID_KWORLD_D39C) },     // 45
	{ USB_DEVICE(USB_VID_KWORLD_1ST, USB_PID_KWORLD_D39E) },     // 46
	{ USB_DEVICE(USB_VID_KWORLD_1ST, USB_PID_KWORLD_E77B) },     // 47
	{ USB_DEVICE(USB_VID_KWORLD_1ST, USB_PID_KWORLD_D3A1) },     // 48
	{ USB_DEVICE(USB_VID_KWORLD_1ST, USB_PID_KWORLD_D3A4) },     // 49
	{ USB_DEVICE(USB_VID_KWORLD_1ST, USB_PID_KWORLD_E41D) },     // 50

	{ USB_DEVICE(USB_VID_GTEK, USB_PID_GTEK_WARM_0837)},         // 51
	{ USB_DEVICE(USB_VID_GTEK, USB_PID_GTEK_WARM_A803)},         // 52
	{ USB_DEVICE(USB_VID_GTEK, USB_PID_GTEK_WARM_B803)},         // 53
	{ USB_DEVICE(USB_VID_GTEK, USB_PID_GTEK_WARM_C803)},         // 54
	{ USB_DEVICE(USB_VID_GTEK, USB_PID_GTEK_WARM_D803)},         // 55
	{ USB_DEVICE(USB_VID_GTEK, USB_PID_GTEK_WARM_C280)},         // 56
	{ USB_DEVICE(USB_VID_GTEK, USB_PID_GTEK_WARM_D286)},         // 57
	{ USB_DEVICE(USB_VID_GTEK, USB_PID_GTEK_WARM_0139)},         // 58
	{ USB_DEVICE(USB_VID_GTEK, USB_PID_GTEK_WARM_A683)},         // 59

	{ USB_DEVICE(USB_VID_LEADTEK, USB_PID_LEADTEK_WARM_1)},      // 60			
	{ USB_DEVICE(USB_VID_LEADTEK, USB_PID_LEADTEK_WARM_2)},      // 61

	{ USB_DEVICE(USB_VID_YUAN, USB_PID_YUAN_WARM)},              // 62

	{ USB_DEVICE(USB_VID_COMPRO, USB_PID_COMPRO_WARM_0620)},     // 63			
	{ USB_DEVICE(USB_VID_COMPRO, USB_PID_COMPRO_WARM_0630)},     // 64			
	{ USB_DEVICE(USB_VID_COMPRO, USB_PID_COMPRO_WARM_0640)},     // 65			
	{ USB_DEVICE(USB_VID_COMPRO, USB_PID_COMPRO_WARM_0650)},     // 66			
	{ USB_DEVICE(USB_VID_COMPRO, USB_PID_COMPRO_WARM_0680)},     // 67			
	{ USB_DEVICE(USB_VID_COMPRO, USB_PID_COMPRO_WARM_9580)},     // 68			
	{ USB_DEVICE(USB_VID_COMPRO, USB_PID_COMPRO_WARM_9550)},     // 69			
	{ USB_DEVICE(USB_VID_COMPRO, USB_PID_COMPRO_WARM_9540)},     // 70			
	{ USB_DEVICE(USB_VID_COMPRO, USB_PID_COMPRO_WARM_9530)},     // 71 //------rtl2832u_6th_properties(6)
	{ USB_DEVICE(USB_VID_COMPRO, USB_PID_COMPRO_WARM_9520)},     // 72

	{ USB_DEVICE(USB_VID_GOLDENBRIDGE, USB_PID_GOLDENBRIDGE_WARM)}, //73	

	{ 0 },
};


MODULE_DEVICE_TABLE(usb, rtl2832u_usb_table);

static struct dvb_usb_device_properties rtl2832u_1st_properties = {

	.num_adapters = 1,
	.adapter = 
	{
		{
			.streaming_ctrl = rtl2832u_streaming_ctrl,
			.frontend_attach = rtl2832u_frontend_attach,
			.fe_ioctl_override = rtl2832u_ioctl_override,
			//parameter for the MPEG2-data transfer 
				.stream = 
				{
					.type = USB_BULK,
					.count = RTD2831_URB_NUMBER,
					.endpoint = 0x01,		//data pipe
					.u = 
					{
						.bulk = 
						{
							.buffersize = RTD2831_URB_SIZE,
						}
					}
				},
		}
	},

	// remote control
	.rc.legacy = {
		.rc_map_table = rc_map_rtl2832u_table,             //user define key map
		.rc_map_size  = ARRAY_SIZE(rc_map_rtl2832u_table), //user define key map size	
		.rc_query     = rtl2832u_rc_query,                 //use define quary function
		.rc_interval  = RT_RC_POLLING_INTERVAL_TIME_MS,		
	},

	.num_device_descs = 9,

	.devices = {
		{ .name = "RTL2832U DVB-T USB DEVICE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[0], NULL },
		},
		{ .name = "RTL2832U DVB-T USB DEVICE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[1], NULL },
		},
		{ .name = "RTL2832U DVB-T USB DEVICE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[2], NULL },
		},
		{ .name = "RTL2832U DVB-T USB DEVICE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[3], NULL },
		},
		{ .name = "RTL2832U DVB-T USB DEVICE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[4], NULL },
		},
		{ .name = "RTL2832U DVB-T USB DEVICE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[5], NULL },
		},		
		{ .name = "RTL2832U DVB-T USB DEVICE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[6], NULL },
		},		
		{ .name = "RTL2832U DVB-T USB DEVICE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[7], NULL },
		},		
		{ .name = "RTL2832U DVB-T USB DEVICE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[8], NULL },
		},
		{ NULL },
	}
};


static struct dvb_usb_device_properties rtl2832u_2nd_properties = {

	.num_adapters = 1,
	.adapter = 
	{
		{
			.streaming_ctrl = rtl2832u_streaming_ctrl,
			.frontend_attach = rtl2832u_frontend_attach,
			.fe_ioctl_override = rtl2832u_ioctl_override,
			//parameter for the MPEG2-data transfer 
				.stream = 
				{
					.type = USB_BULK,
					.count = RTD2831_URB_NUMBER,
					.endpoint = 0x01,		//data pipe
					.u = 
					{
						.bulk = 
						{
							.buffersize = RTD2831_URB_SIZE,
						}
					}
				},
		}
	},

	// remote control
	.rc.legacy = {
		.rc_map_table = rc_map_rtl2832u_table,             //user define key map
		.rc_map_size  = ARRAY_SIZE(rc_map_rtl2832u_table), //user define key map size	
		.rc_query     = rtl2832u_rc_query,                 //use define quary function
		.rc_interval  = RT_RC_POLLING_INTERVAL_TIME_MS,		
	},
	
	.num_device_descs = 9,

	.devices = {
		{ .name = "RTL2832U DVB-T USB DEVICE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[9], NULL },
		},
		{ .name = "RTL2832U DVB-T USB DEVICE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[10], NULL },
		},
		{ .name = "RTL2832U DVB-T USB DEVICE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[11], NULL },
		},
		{ .name = "RTL2832U DVB-T USB DEVICE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[12], NULL },
		},
		{ .name = "RTL2832U DVB-T USB DEVICE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[13], NULL },
		},
		{ .name = "RTL2832U DVB-T USB DEVICE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[14], NULL },
		},
		{ .name = "RTL2832U DVB-T USB DEVICE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[15], NULL },
		},
		{ .name = "DK DONGLE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[16], NULL },
		},
		{ .name = "DK DONGLE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[17], NULL },
		},
		{ NULL },
	}
};



static struct dvb_usb_device_properties rtl2832u_3th_properties = {

	.num_adapters = 1,
	.adapter = 
	{
		{
			.streaming_ctrl = rtl2832u_streaming_ctrl,
			.frontend_attach = rtl2832u_frontend_attach,
			.fe_ioctl_override = rtl2832u_ioctl_override,
			//parameter for the MPEG2-data transfer 
				.stream = 
				{
					.type = USB_BULK,
					.count = RTD2831_URB_NUMBER,
					.endpoint = 0x01,		//data pipe
					.u = 
					{
						.bulk = 
						{
							.buffersize = RTD2831_URB_SIZE,
						}
					}
				},
		}
	},

	// remote control
	.rc.legacy = {
		.rc_map_table = rc_map_rtl2832u_table,             //user define key map
		.rc_map_size  = ARRAY_SIZE(rc_map_rtl2832u_table), //user define key map size	
		.rc_query     = rtl2832u_rc_query,                 //use define quary function
		.rc_interval  = RT_RC_POLLING_INTERVAL_TIME_MS,		
	},
	
	.num_device_descs = 9,

	.devices = {
		{ .name = "DK DONGLE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[18], NULL },
		},
		{ .name = "DK DONGLE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[19], NULL },
		},
		{ .name = "DK DONGLE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[20], NULL },
		},
		{
		  .name = "DK DONGLE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[21], NULL },
		},
		{
		  .name = "DK DONGLE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[22], NULL },
		},
		{
		  .name = "DK DONGLE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[23], NULL },
		},
		{
		  .name = "DK DONGLE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[24], NULL },
		},
		{
		  .name = "DK DONGLE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[25], NULL },
		},
		{
		  .name = "DK DONGLE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[26], NULL },
		}
	}
};


static struct dvb_usb_device_properties rtl2832u_4th_properties = {

	.num_adapters = 1,
	.adapter = 
	{
		{
			.streaming_ctrl = rtl2832u_streaming_ctrl,
			.frontend_attach = rtl2832u_frontend_attach,
			.fe_ioctl_override = rtl2832u_ioctl_override,
			//parameter for the MPEG2-data transfer 
				.stream = 
				{
					.type = USB_BULK,
					.count = RTD2831_URB_NUMBER,
					.endpoint = 0x01,		//data pipe
					.u = 
					{
						.bulk = 
						{
							.buffersize = RTD2831_URB_SIZE,
						}
					}
				},
		}
	},

	// remote control
	.rc.legacy = {
		.rc_map_table = rc_map_rtl2832u_table,             //user define key map
		.rc_map_size  = ARRAY_SIZE(rc_map_rtl2832u_table), //user define key map size	
		.rc_query     = rtl2832u_rc_query,                 //use define quary function
		.rc_interval  = RT_RC_POLLING_INTERVAL_TIME_MS,		
	},
	
	.num_device_descs = 9,

	.devices = {
		{ .name = "DK DONGLE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[27], NULL },
		},
		{ .name = "DK DONGLE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[28], NULL },
		},
		{ .name = "Terratec Cinergy T Stick Black",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[29], NULL },
		},
		{
		  .name = "Terratec Cinergy T Stick Black",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[30], NULL },
		},
		{
		  .name = "USB DVB-T Device",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[31], NULL },
		},
		{
		  .name = "USB DVB-T Device",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[32], NULL },
		},
		
		{
		  .name = "USB DVB-T Device",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[33], NULL },
		},
				
		{
		  .name = "RTL2832U DVB-T USB DEVICE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[34], NULL },
		},
		
		{
		  .name = "RTL2832U DVB-T USB DEVICE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[35], NULL },
		},				
		
		
	}
};

static struct dvb_usb_device_properties rtl2832u_5th_properties = {

	.num_adapters = 1,
	.adapter = 
	{
		{
			.streaming_ctrl = rtl2832u_streaming_ctrl,
			.frontend_attach = rtl2832u_frontend_attach,
			.fe_ioctl_override = rtl2832u_ioctl_override,
			//parameter for the MPEG2-data transfer 
				.stream = 
				{
					.type = USB_BULK,
					.count = RTD2831_URB_NUMBER,
					.endpoint = 0x01,		//data pipe
					.u = 
					{
						.bulk = 
						{
							.buffersize = RTD2831_URB_SIZE,
						}
					}
				},
		}
	},

	// remote control
	.rc.legacy = {
		.rc_map_table = rc_map_rtl2832u_table,             //user define key map
		.rc_map_size  = ARRAY_SIZE(rc_map_rtl2832u_table), //user define key map size	
		.rc_query     = rtl2832u_rc_query,                 //use define quary function
		.rc_interval  = RT_RC_POLLING_INTERVAL_TIME_MS,		
	},
	
	.num_device_descs = 9,

	.devices = {
		{ .name = "RTL2832U DVB-T USB DEVICE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[36], NULL },
		},
		{ .name = "USB DVB-T DEVICE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[37], NULL },
		},
		{ .name = "USB DVB-T DEVICE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[38], NULL },
		},
		{
		  .name = "USB DVB-T DEVICE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[39], NULL },
		},
		{
		  .name = "USB DVB-T DEVICE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[40], NULL },
		},
		{
		  .name = "USB DVB-T DEVICE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[41], NULL },
		},
		
		{
		  .name = "USB DVB-T DEVICE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[42], NULL },
		},
				
		{
		  .name = "USB DVB-T DEVICE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[43], NULL },
		},
		
		{
		  .name = "USB DVB-T DEVICE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[44], NULL },
		},				
		
		
	}
};

static struct dvb_usb_device_properties rtl2832u_6th_properties = {

	.num_adapters = 1,
	.adapter = 
	{
		{
			.streaming_ctrl = rtl2832u_streaming_ctrl,
			.frontend_attach = rtl2832u_frontend_attach,
			.fe_ioctl_override = rtl2832u_ioctl_override,
			//parameter for the MPEG2-data transfer 
				.stream = 
				{
					.type = USB_BULK,
					.count = RTD2831_URB_NUMBER,
					.endpoint = 0x01,		//data pipe
					.u = 
					{
						.bulk = 
						{
							.buffersize = RTD2831_URB_SIZE,
						}
					}
				},
		}
	},

	// remote control
	.rc.legacy = {
		.rc_map_table = rc_map_rtl2832u_table,             //user define key map
		.rc_map_size  = ARRAY_SIZE(rc_map_rtl2832u_table), //user define key map size	
		.rc_query     = rtl2832u_rc_query,                 //use define quary function
		.rc_interval  = RT_RC_POLLING_INTERVAL_TIME_MS,		
	},
		
	.num_device_descs = 9,

	.devices = {
		{ .name = "USB DVB-T DEVICE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[45], NULL },
		},
		{ .name = "USB DVB-T DEVICE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[46], NULL },
		},
		{ .name = "USB DVB-T DEVICE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[47], NULL },
		},
		{
		  .name ="USB DVB-T DEVICE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[48], NULL },
		},
		{
		  .name = "USB DVB-T DEVICE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[49], NULL },
		},
		{
		  .name = "USB DVB-T DEVICE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[50], NULL },
		},
		{
		  .name ="DVB-T TV Stick",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[51], NULL },
		},
		{
		  .name = "DVB-T TV Stick",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[52], NULL },
		},
		{
		  .name = "DVB-T TV Stick",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[53], NULL },
		},
		
		{ NULL },				
		
		
	}
};

static struct dvb_usb_device_properties rtl2832u_7th_properties = {

	.num_adapters = 1,
	.adapter = 
	{
		{
			.streaming_ctrl = rtl2832u_streaming_ctrl,
			.frontend_attach = rtl2832u_frontend_attach,
			.fe_ioctl_override = rtl2832u_ioctl_override,
			//parameter for the MPEG2-data transfer 
				.stream = 
				{
					.type = USB_BULK,
					.count = RTD2831_URB_NUMBER,
					.endpoint = 0x01,		//data pipe
					.u = 
					{
						.bulk = 
						{
							.buffersize = RTD2831_URB_SIZE,
						}
					}
				},
		}
	},

	//remote control
	.rc.legacy = {
		.rc_map_table = rc_map_rtl2832u_table,             //user define key map
		.rc_map_size  = ARRAY_SIZE(rc_map_rtl2832u_table), //user define key map size	
		.rc_query     = rtl2832u_rc_query,                 //use define quary function
		.rc_interval  = RT_RC_POLLING_INTERVAL_TIME_MS,		
	},
	
	.num_device_descs = 9,

	.devices = {
		{ .name = "DVB-T TV Stick",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[54], NULL },
		},
		{ .name = "DVB-T TV Stick",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[55], NULL },
		},
		{ .name = "DVB-T TV Stick",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[56], NULL },
		},
		{
		  .name ="DVB-T TV Stick",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[57], NULL },
		},
		{ .name = "DVB-T TV Stick",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[58], NULL },
		},
		{ .name = "DVB-T TV Stick",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[59], NULL },
		},
		{ .name = "USB DVB-T Device",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[60], NULL },
		},
		{
		  .name ="USB DVB-T Device",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[61], NULL },
		},
		{
		  .name ="USB DVB-T Device",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[62], NULL },
		},

		{ NULL },				
	}
};

static struct dvb_usb_device_properties rtl2832u_8th_properties = {

	.num_adapters = 1,
	.adapter = 
	{
		{
			.streaming_ctrl = rtl2832u_streaming_ctrl,
			.frontend_attach = rtl2832u_frontend_attach,
			.fe_ioctl_override = rtl2832u_ioctl_override,
			//parameter for the MPEG2-data transfer 
				.stream = 
				{
					.type = USB_BULK,
					.count = RTD2831_URB_NUMBER,
					.endpoint = 0x01,		//data pipe
					.u = 
					{
						.bulk = 
						{
							.buffersize = RTD2831_URB_SIZE,
						}
					}
				},
		}
	},

	//remote control
	.rc.legacy = {
		.rc_map_table = rc_map_rtl2832u_table,             //user define key map
		.rc_map_size  = ARRAY_SIZE(rc_map_rtl2832u_table), //user define key map size	
		.rc_query     = rtl2832u_rc_query,                 //use define quary function
		.rc_interval  = RT_RC_POLLING_INTERVAL_TIME_MS,		
	},
	
	.num_device_descs = 9,

	.devices = {
		{ .name = "VideoMate DTV",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[63], NULL },
		},
		{ .name = "VideoMate DTV",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[64], NULL },
		},
		{ .name = "VideoMate DTV",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[65], NULL },
		},
		{
		  .name ="VideoMate DTV",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[66], NULL },
		},
		{ .name = "VideoMate DTV",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[67], NULL },
		},
		{ .name = "VideoMate DTV",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[68], NULL },
		},
		{ .name = "VideoMate DTV",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[69], NULL },
		},
		{
		  .name ="VideoMate DTV",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[70], NULL },
		},
		{
		  .name ="VideoMate DTV",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[71], NULL },
		},

		{ NULL },				
	}
};

static struct dvb_usb_device_properties rtl2832u_9th_properties = {

	.num_adapters = 1,
	.adapter = 
	{
		{
			.streaming_ctrl = rtl2832u_streaming_ctrl,
			.frontend_attach = rtl2832u_frontend_attach,
			.fe_ioctl_override = rtl2832u_ioctl_override,
			//parameter for the MPEG2-data transfer 
				.stream = 
				{
					.type = USB_BULK,
					.count = RTD2831_URB_NUMBER,
					.endpoint = 0x01,		//data pipe
					.u = 
					{
						.bulk = 
						{
							.buffersize = RTD2831_URB_SIZE,
						}
					}
				},
		}
	},

	//remote control
	.rc.legacy = {
		.rc_map_table = rc_map_rtl2832u_table,             //user define key map
		.rc_map_size  = ARRAY_SIZE(rc_map_rtl2832u_table), //user define key map size	
		.rc_query     = rtl2832u_rc_query,                 //use define quary function
		.rc_interval  = RT_RC_POLLING_INTERVAL_TIME_MS,		
	},
	
	.num_device_descs = 2,

	.devices = {
		{ .name = "VideoMate DTV",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[72], NULL },
		},
		{ .name = "DVB-T TV Stick",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[73], NULL },
		},
		{ NULL },				
	}
};


static struct usb_driver rtl2832u_usb_driver = {
	.name       = "dvb_usb_rtl2832u",
	.probe      = rtl2832u_usb_probe,
	.disconnect = rtl2832u_usb_disconnect,
	.id_table   = rtl2832u_usb_table,
};


static s32 __init rtl2832u_usb_module_init(void)
{
	s32 result =0 ;
	
	deb_info("+info debug open_%s\n", __FUNCTION__);
	if((result = usb_register(&rtl2832u_usb_driver))) {
		err("usb_register failed. (%d)",result);
		return result;
	}

	return 0;
}

static void __exit rtl2832u_usb_module_exit(void)
{
	usb_deregister(&rtl2832u_usb_driver);
	return ;	
}


module_init(rtl2832u_usb_module_init);
module_exit(rtl2832u_usb_module_exit);

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//Remake : based on linux kernel : 3.0.0-12  ubuntu 11.04 Xgaz						// Data:2011/10/18											//	
//////////////////////////////////////////////////////////////////////////////////////////////////////////

MODULE_AUTHOR("Realtek");
MODULE_AUTHOR("Chialing Lu <chialing@realtek.com>");
MODULE_AUTHOR("Dean Chung <DeanChung@realtek.com>");
MODULE_DESCRIPTION("Driver for the RTL2832U DVB-T / RTL2836 DTMB USB2.0 device");
MODULE_VERSION("2.2.1");
MODULE_LICENSE("GPL");

