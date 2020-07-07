///////////////////////////////////////////////////////
//
//  Programmed by 이양희교수
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

//패킷 갯수
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

  int32_t Output32;   //32bit형  모터현재출력(제어기입력)
  int32_t Target32;    //32bit형  모터목표출력(제어기입력)
  float    OutputFloat;  //float  모터현재출력(제어기입력)
  float    TargetFloat;  //float  모터목표출력(제어기입력)

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

   float    f;//PWM 실수형 계산값
   int16_t  U; //PWM 정수형 출력값 <=U_MAX(100)
   int16_t  u;
}PidController1;

/*
링버퍼에서 패킷 추출
atoi, atof, strtod, strtof 등 연습
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
atoi, atof, strtod, strtof 등 연습
https://www.ibm.com/support/knowledgecenter/ko/ssw_ibm_i_73/rtref/strtod.htm
https://dojang.io/mod/page/view.php?id=387
*/
void PacketParsing(typePacket *packet)
{ 
  int len = strlen(packet->buffer);
  int16_t count=0;
  int32_t target;
  char *end;
  //toupper 문자를 무조건 대문자로 바꾸어 리턴
  switch( toupper(packet->buffer[0]) ){
     case 'K' ://Kp값 변경
         //strtof : string to float로 변환하는 함수, 이를 반복적으로 사용가능  strtof 함수는 수로 구성한 문자열을 계산하여 float 형식 값을 반환합니다. 그리고 계산할 수 없는 문자를 발견하면 두 번째 입력 인자로 받은 endptr이 가리키는 곳에 설정합니다. 

       if( RunState==STATE_IDLE){   //Idle모드 일때만 변경가능
         PidParam.Kp = strtof( packet->buffer+1, &end );
         PidParam.Ki = strtof(end, &end );
         PidParam.Kd = strtof(end, &end );
         
          //추후 삭제
         printf("Kp=%1c %10.3f %10.3f %10.3f \r\n", packet->buffer[0], PidParam.Kp, PidParam.Ki, PidParam.Kd);
       }
       
         break;

     case 'E' ://Encoder값 변경
        if( RunState==STATE_IDLE){   //Idle모드 일때만 변경가능  
       count = atoi( packet->buffer+1 );
         //엔코더값을 count로 변경
          __HAL_TIM_SET_COUNTER(&htim4,   count); 
         //추후 삭제
         printf("Encoder=%1c %10d\r\n", packet->buffer[0], count);
        }
         break;
         
     case 'R' ://// idlemode=>run
         RunState = STATE_RUN_SAVE;//Run_Save 모드 전환
          //추후 삭제
          //초기화  .....
         HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin,GPIO_PIN_SET);  
        
        printf("Run\r\n");
         break; 

     case 'T' ://Target
         target = atoi( packet->buffer+1 );         
          PidController1.Target32= target;
          //추후 삭제
          printf("Target=%1c %8d\r\n", packet->buffer[0], PidController1.Target32);;
        break; 

      case 'P' ://Stop
        // Idle mode 
        //추후 삭제
        RunState =STATE_IDLE;  // Idle mode
         printf("Stop\r\n");
           HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin,GPIO_PIN_RESET);   // 확인용
           
         break; 
         
          case 'I' ://Stop//Idle mode
        // Idle mode 
        //추후 삭제
           RunState =STATE_IDLE;  // Idle mode
         printf("Stop\r\n");
           HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin,GPIO_PIN_RESET);   // 확인용
         break; 
  } 
}

