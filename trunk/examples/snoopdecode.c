#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_LINE 1024

char line[MAX_LINE] =3D "";
FILE* file;

enum
{
        DIR_UNKNOWN =3D 0,
        DIR_TO_DEVICE,
        DIR_FROM_DEVICE
};

enum
{
        FUNC_UNKNOWN =3D 0,
        FUNC_CONTROL_TXFER,
        FUNC_CLASS_INTERFACE,
        FUNC_GET_DESC,
        FUNC_SELECT_CONFIG,
        FUNC_GET_DESC_FROM_IFACE,
        FUNC_BULK_TXFER,
};

enum
{
        REQ_GET_REPORT =3D 1,
        REQ_GET_IDLE =3D 2,
        REQ_GET_PROTOCOL =3D 3,
        REQ_SET_REPORT =3D 9,
        REQ_SET_IDLE =3D 10,
        REQ_SET_PROTOCOL =3D 11
};

struct urb
{
        int seq;
        unsigned long len;
        unsigned char* data;
        unsigned long flags;
        unsigned long index;
        unsigned long type;
        unsigned long reserved;
        unsigned long request;
        unsigned long value;
        int time;
        int dir;
        int func;
};

struct rpt
{
        struct rpt* next;
        unsigned char* data;
        int seq;
        unsigned short len;
        unsigned char id;
};

struct rpt* rpts =3D NULL;

/*
[197 ms]  >>>  URB 1 going down  >>>=20
-- URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE:
  TransferBufferLength =3D 00000012
  TransferBuffer       =3D 864c9208
  TransferBufferMDL    =3D 00000000
  Index                =3D 00000000
  DescriptorType       =3D 00000001 (USB_DEVICE_DESCRIPTOR_TYPE)
  LanguageId           =3D 00000000
[203 ms] UsbSnoop - MyInternalIOCTLCompletion(f7b91db0) : fido=3D00000000, =
Irp=3D863df850, Context=3D86405d10, IRQL=3D2

[203 ms]  <<<  URB 1 coming back  <<<=20
-- URB_FUNCTION_CONTROL_TRANSFER:
  PipeHandle           =3D 863e4150
  TransferFlags        =3D 0000000b (USBD_TRANSFER_DIRECTION_IN, USBD_SHORT=
_TRANSFER_OK)
  TransferBufferLength =3D 00000012
  TransferBuffer       =3D 864c9208
  TransferBufferMDL    =3D 8640f108
    00000000: 12 01 10 01 00 00 00 08 1d 05 02 00 06 00 03 01
    00000010: 02 01
  UrbLink              =3D 00000000
*/

/*
[59563 ms]  >>>  URB 346 going down  >>>=20
-- URB_FUNCTION_CLASS_INTERFACE:
  TransferFlags          =3D 00000001 (USBD_TRANSFER_DIRECTION_IN, ~USBD_SH=
ORT_TRANSFER_OK)
  TransferBufferLength =3D 00000005
  TransferBuffer       =3D f7f12ba0
  TransferBufferMDL    =3D 00000000
  UrbLink                 =3D 00000000
  RequestTypeReservedBits =3D 00000022
  Request                 =3D 00000001
  Value                   =3D 0000032f
  Index                   =3D 00000000
[59567 ms] UsbSnoop - MyInternalIOCTLCompletion(f7b91db0) : fido=3D86397288=
, Irp=3D85ff4b40, Context=3D85f4b3d8, IRQL=3D2
[59567 ms]  <<<  URB 346 coming back  <<<=20
-- URB_FUNCTION_CONTROL_TRANSFER:
  PipeHandle           =3D 863e4150
  TransferFlags        =3D 0000000b (USBD_TRANSFER_DIRECTION_IN, USBD_SHORT=
_TRANSFER_OK)
  TransferBufferLength =3D 00000002
  TransferBuffer       =3D f7f12ba0
  TransferBufferMDL    =3D 85f102f0
    00000000: 2f 02
  UrbLink              =3D 00000000
  SetupPacket          =3D
    00000000: a1 01 2f 03 00 00 05 00
*/

int fetch_line()
{
        return !!fgets(line, sizeof(line), file);
}

int find_urb()
{
        do
        {
                if (line[0] =3D=3D '[' && strstr(line, " URB ") &&
                   (strstr(line, "going down") || strstr(line, "coming back")))
                {
                        return 1;
                }
        }
        while( fetch_line() );
=09
        return 0;
}

int init_urb(struct urb* urb)
{
        memset(urb, 0, sizeof(*urb));
=09
        if (!find_urb())
                return 0;
=09
        urb->time =3D atoi(line+1);
=09
        if (strstr(line, "going down"))
                urb->dir =3D DIR_TO_DEVICE;
        else if(strstr(line, "coming back"))
                urb->dir =3D DIR_FROM_DEVICE;
        else
                urb->dir =3D DIR_UNKNOWN;
        =09
        urb->seq =3D atoi(strstr(line,"URB ")+4);

        return 1;
}

unsigned long get_hex_value()
{
        char* ptr =3D strchr(line, '=3D');
        if (!ptr)
                return 0xffff;

        return strtoul(ptr+2, NULL, 16);
}

char* parse_data_dump(int len)
{
        int count =3D 0;
        char* ptr;
=09
        char* data =3D malloc(len);=09
        if (!data)
                return NULL;
        =09
        while(count < len)
        {
                if ((count % 16) =3D=3D 0)
                {
                        if (count)
                        {
                                if (!fetch_line())
                                {
                                        free(data);
                                        return NULL;
                                }
                        }

                        ptr =3D strchr(line, ':');
                        if (!ptr)
                        {
                                free(data);
                                return NULL;
                        }

                        ptr +=3D 2;
                }

                data[count++] =3D strtoul(ptr, NULL, 16);

                ptr +=3D 3;
        }
=09
        return data;
}

void parse_urb_body(struct urb* urb)
{
        int setup_packet =3D 0;
=09
        while (fetch_line())
        {
                if (line[0] =3D=3D '[')
                        break;
                =09
                if (strstr(line, "TransferBufferLength"))
                {
                        urb->len =3D get_hex_value();
                }
                else if (strstr(line, "TransferFlags"))
                {
                        urb->flags =3D get_hex_value();
                }
                else if (strstr(line, "Index"))
                {
                        urb->index =3D get_hex_value();
                }
                else if (strstr(line, "DescriptorType"))
                {
                        urb->type =3D get_hex_value();
                }
                else if (strstr(line, "RequestTypeReservedBits"))
                {
                        urb->reserved =3D get_hex_value();
                }
                else if (strstr(line, "Request"))
                {
                        urb->request =3D get_hex_value();
                }
                else if (strstr(line, "Value"))
                {
                        urb->value =3D get_hex_value();
                }
                else if (strstr(line, "SetupPacket"))
                {
                        setup_packet =3D 1;
                }
                else if (strstr(line, "00000000:"))
                {
                        if (!setup_packet && !urb->data)
                                urb->data =3D parse_data_dump(urb->len);
                }
                else if (strstr(line, "-- URB_FUNCTION_"))
                {
                        if (strstr(line, "URB_FUNCTION_CONTROL_TRANSFER"))
                                urb->func =3D FUNC_CONTROL_TXFER;
                        else if (strstr(line, "URB_FUNCTION_CLASS_INTERFACE"))
                                urb->func =3D FUNC_CLASS_INTERFACE;
                        else if (strstr(line, "URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE"))
                                urb->func =3D FUNC_GET_DESC;
                        else if (strstr(line, "URB_FUNCTION_SELECT_CONFIGURATION"))
                                urb->func =3D FUNC_SELECT_CONFIG;
                        else if (strstr(line, "URB_FUNCTION_GET_DESCRIPTOR_FROM_INTERFACE"))
                                urb->func =3D FUNC_GET_DESC_FROM_IFACE;
                        else if (strstr(line, "URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER"))
                                urb->func =3D FUNC_BULK_TXFER;
                        else
                        {
                                urb->func =3D FUNC_UNKNOWN;
                                printf("Unknown FUNC: %s\n", line);
                                exit(0);
                        }
                }
        }
}

void free_urb(struct urb* urb)
{
        free(urb->data);
}

void print_urb(struct urb* urb)
{
        int x;
        printf("[%08d ms] %05d %c ", urb->time, urb->seq,
                (urb->dir =3D=3D DIR_TO_DEVICE) ? '>' : (urb->dir =3D=3D DIR_FROM_DEVICE)=
 ? '<' : '?');
        =09
        printf("req=3D%04lx value=3D%04lx", urb->request, urb->value);
=09
        if( urb->data )
        {
                printf(" [");
                for(x=3D0; x<urb->len; x++)
                {
                        printf( "%02x", urb->data[x] );
                        if (x < urb->len-1)
                                printf(" ");
                }
                printf("]");
        }
=09
        printf("\n");
}

struct rpt* find_report_by_seq(int seq)
{
        struct rpt* rpt =3D rpts;

        while( rpt )
        {
                if (rpt->seq =3D=3D seq)
                        break;
                =09
                rpt =3D rpt->next;
        }
=09
        return rpt;
}

struct rpt* find_report_by_id(unsigned char id)
{
        struct rpt* rpt =3D rpts;

        while( rpt )
        {
                if (rpt->id =3D=3D id)
                        break;

                rpt =3D rpt->next;
        }
=09
        return rpt;
}

void del_report(struct urb* urb)
{
        unsigned char id =3D urb->data[0];
        struct rpt* rpt =3D rpts;
        struct rpt** prev =3D &rpts;
=09
        while( rpt )
        {
                if (rpt->id =3D=3D id)
                        break;
                =09
                prev =3D &(rpt->next);
                rpt =3D rpt->next;
        }
=09
        if (rpt)
        {
                *prev =3D rpt->next;
                free(rpt);
        }
}

void add_report(struct urb* urb)
{
        struct rpt* rpt;
        unsigned char id;

        if (urb->data)
                id =3D urb->data[0];
        else
                id =3D urb->value & 0xff;

        if ((rpt =3D find_report_by_id(id)))
        {
                rpt->seq =3D urb->seq;
                return;
        }

        rpt =3D malloc(sizeof(*rpt));
        memset(rpt, 0, sizeof(*rpt));

        rpt->id =3D id;
        rpt->seq =3D urb->seq;

        rpt->next =3D rpts;
        rpts =3D rpt;
}

int check_and_update_report(struct urb* urb)
{
        struct rpt* rpt;

        rpt =3D find_report_by_seq(urb->seq);
        if (!rpt)
                return 0;

        if (rpt->data && !memcmp(rpt->data, urb->data+1, urb->len-1))
                return 0;

        free(rpt->data);
        rpt->len =3D urb->len-1;
        rpt->data =3D malloc(rpt->len);
        memcpy(rpt->data, urb->data+1, rpt->len);

        return 1;
}

void display_report(struct urb* urb)
{
        unsigned long data;
        char buf[7];

        memset(buf, ' ', sizeof(buf)-1);=09

        if (urb->len =3D=3D 2)
        {
                data =3D urb->data[1];
                sprintf(buf+4, "%02lx", data);
        }
        else if(urb->len =3D=3D 3)
        {
                data =3D urb->data[1] | (urb->data[2] << 8);
                sprintf(buf+2, "%04lx", data);
        }
        else if(urb->len =3D=3D 4)
        {
                data =3D urb->data[1] | (urb->data[2] << 8) | (urb->data[3] << 16);
                sprintf(buf, "%06lx", data);
        }

        printf( "[%04d s] %05d %c %s 0x%02x %s (%lu)\n", urb->time/1000, urb->seq,=
 urb->func =3D=3D FUNC_BULK_TXFER ? '*' : ' ', urb->dir=3D=3DDIR_TO_DEVICE =
? "WRITE" : " READ",
                urb->data[0], buf, data);
}

void update_reports(struct urb* urb)
{
        if (urb->dir =3D=3D DIR_TO_DEVICE)
        {
                // Only care about class requests
                if (urb->func !=3D FUNC_CLASS_INTERFACE)
                        return;

                if (urb->request =3D=3D REQ_GET_REPORT)
                {
                        add_report(urb);
                }
                else if (urb->request =3D=3D REQ_SET_REPORT)
                {
                        display_report(urb);
                        del_report(urb);
                }
        }
        else
        {
                if (urb->func =3D=3D FUNC_CONTROL_TXFER)
                {
                        if (check_and_update_report(urb))
                                display_report(urb);
                }
                else if (urb->data && urb->func =3D=3D FUNC_BULK_TXFER)
                {
                        add_report(urb);
                        if (check_and_update_report(urb))
                                display_report(urb);
                }
        }
}

int main(int argc, char* argv[])
{
        struct urb urb;

        file =3D stdin;

        while(init_urb(&urb))
        {
                parse_urb_body(&urb);
        =09
                update_reports(&urb);
        =09
                free_urb(&urb);
        }

        return 0;
}
