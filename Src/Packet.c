///////////////////////////////////////////////////////
//
//  Programmed by �̾��񱳼�
//
////////////////////////////////////////////////////
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "main.h"

#include "DcMotor.h"
#include "DcMotorExtern.h"

#include "circular-byte-buffer.h"

#include "Packet.h"

//��Ŷ ����
uint16_t countPacket=0;

extern struct{
   float Kp;
   float Ki;
   float Kd;
}PidParam;

extern enum {
  STATE_IDLE = 0,
  STATE_RUN_SAVE,
  STATE_NORUN_PRINT,
  STATE_RUN_ONLY,  
} RunState;

extern TIM_HandleTypeDef htim4;

extern  struct{
   int loop;

  int32_t Output32;   //32bit��  �����������(������Է�)
  int32_t Target32;    //32bit��  ���͸�ǥ���(������Է�)
  float    OutputFloat;  //float  �����������(������Է�)
  float    TargetFloat;  //float  ���͸�ǥ���(������Է�)

   int32_t  Error32;
   int32_t  ErrorSum32;
   int32_t  ErrorOld32;
   int32_t  ErrorDiff32;
   int32_t  ErrorDiffOld32;

   float  ErrorFloat;
   float  ErrorSumFloat;
   float  Errorfloat;  
   float  ErrorOldFloat;
   float  ErrorDiffFloat;
   float  ErrorDiffOldFloat;

   float    f;//PWM �Ǽ��� ��갪
   int16_t  U; //PWM ������ ��°� <=U_MAX(100)
   int16_t  u;
}PidController1;

/*
�����ۿ��� ��Ŷ ����
atoi, atof, strtod, strtof �� ����
https://www.ibm.com/support/knowledgecenter/ko/ssw_ibm_i_73/rtref/strtod.htm
https://dojang.io/mod/page/view.php?id=387
*/
int PacketExtractFromBuffer(circ_bbuf_t *circ_buff, typePacket *packet)
{
  int len=0;
  uint8_t ch;
//  circ_bbuf_pop(circ_bbuf_t *c, uint8_t *data);
  len=0;
  do{
    circ_bbuf_pop(circ_buff, &ch);
    packet->buffer[len++] = ch;
    //= strlen(packet->buffer);
  } while(ch);
  packet->len= strlen(packet->buffer);
  countPacket--;
  return len;
}

/*
atoi, atof, strtod, strtof �� ����
https://www.ibm.com/support/knowledgecenter/ko/ssw_ibm_i_73/rtref/strtod.htm
https://dojang.io/mod/page/view.php?id=387
*/
void PacketParsing(typePacket *packet)
{ 
  int len = strlen(packet->buffer);
  int16_t count=0;
  int32_t target;
  char *end;
  //toupper ���ڸ� ������ �빮�ڷ� �ٲپ� ����
  switch( toupper(packet->buffer[0]) ){
     case 'K' ://Kp�� ����
         //strtof : string to float�� ��ȯ�ϴ� �Լ�, �̸� �ݺ������� ��밡��  strtof �Լ��� ���� ������ ���ڿ��� ����Ͽ� float ���� ���� ��ȯ�մϴ�. �׸��� ����� �� ���� ���ڸ� �߰��ϸ� �� ��° �Է� ���ڷ� ���� endptr�� ����Ű�� ���� �����մϴ�. 

       if( RunState==STATE_IDLE){   //Idle��� �϶��� ���氡��
         PidParam.Kp = strtof( packet->buffer+1, &end );
         PidParam.Ki = strtof(end, &end );
         PidParam.Kd = strtof(end, &end );
         
          //���� ����
         printf("Kp=%1c %10.3f %10.3f %10.3f \r\n", packet->buffer[0], PidParam.Kp, PidParam.Ki, PidParam.Kd);
       }
       
         break;

     case 'E' ://Encoder�� ����
        if( RunState==STATE_IDLE){   //Idle��� �϶��� ���氡��  
       count = atoi( packet->buffer+1 );
         //���ڴ����� count�� ����
          __HAL_TIM_SET_COUNTER(&htim4,   count); 
         //���� ����
         printf("Encoder=%1c %10d\r\n", packet->buffer[0], count);
        }
         break;
         
     case 'R' ://// idlemode=>run
         RunState = STATE_RUN_SAVE;//Run_Save ��� ��ȯ
          //���� ����
          //�ʱ�ȭ  .....
         HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin,GPIO_PIN_SET);  
        
        printf("Run\r\n");
         break; 

     case 'T' ://Target
         target = atoi( packet->buffer+1 );         
          PidController1.Target32= target;
          //���� ����
          printf("Target=%1c %8d\r\n", packet->buffer[0], PidController1.Target32);;
        break; 

      case 'P' ://Stop
        // Idle mode 
        //���� ����
        RunState =STATE_IDLE;  // Idle mode
         printf("Stop\r\n");
           HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin,GPIO_PIN_RESET);   // Ȯ�ο�
           
         break; 
         
          case 'I' ://Stop//Idle mode
        // Idle mode 
        //���� ����
           RunState =STATE_IDLE;  // Idle mode
         printf("Stop\r\n");
           HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin,GPIO_PIN_RESET);   // Ȯ�ο�
         break; 
  } 
}

