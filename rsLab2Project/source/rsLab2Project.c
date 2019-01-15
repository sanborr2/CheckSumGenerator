/*****************************************************************************
* EE344 Lab 1 Demo Code
*	Displays 'Hello[n]' on the terminal, where n starts at zero and increments
*   each time through the loop. THe user selects 'Enter' to go through the loop
*   again, and 'q' to quit.
*
* Todd Morton, 10/04/2017
*
* Modified by: Robert Sanborn, 10/6/18
*   > Code Utilized for Lab 2 Project
*   > New Program created that obtains two addresses from user via terminal
*       and as long as inputs are of the form XXXXXXXX (8 hexadecimal
*       characters with either lowercase or uppercase letters) and the user
*       specified higher address is greater than or equal to the user specified
*       lower address, a checksum is calculated for the memory block. The
*       calculations are displayed on the terminal screen.
*****************************************************************************/
#include "MCUType.h"               /* Include project header file       */
#include "BasicIO.h"
#include "K65TWR_ClkCfg.h"



/**********************************************************************************
* Defined Constants
**********************************************************************************/
#define USER_IN_LN             9U     /* Define length of user string input*/

#define GET_LOW_ADDR          10U          /* Define states of the program */
#define GET_HIGH_ADDR         20U
#define CS_DISPLAY            30U

#define NO_ERROR               0           /* Error codes for CheckUserInput*/
#define NON_HEX_CHAR          2U
#define NO_INPUT              3U
#define INPUT_TOO_LONG1       4U
#define INPUT_TOO_LONG2       5U
#define NON_HEX_AND_TOO_LONG  6U



#define CRC_CTRL_CLR_WAS ~0x02000000
#define GPOLYNOMIAL       0x00008005
#define LOW_SHORT         0x0000FFFF

/**********************************************************************************
* Function Prototypes
**********************************************************************************/

/**********************************************************************************
* CheckUserInput() - verifies if user gave valid 32 bit Hex String for address
*
* Description:  A routine that obtains a string input from user and checks
*               the return values of BIOGetString and BIOHexStrgtoWord
*               to see if user gave valid input. For input to be valid, form
*               must be XXXXXXXX, where each character is a hex value. The
*               32bit pointer will be changed
*
* Return Value: 0       -> No error detected
*               1       -> Error
*
* Arguments:    NewAddr is a pointer to the 32 bit unsigned int
*               that will contain the hex value passed by user if valid.
**********************************************************************************/
static INT8U CheckUserInput(INT32U *NewAddr);

/**********************************************************************************
* CalcCRC_16() -ascertains checksum for memory block indicated by starting and
*               ending addresses
*
* Description:  Loads every byte from starting address to ending address
*               and calculates the CRC for that memory block
*
* Return Value: crc, 16 bit unsigned integer, the first 16 bits of the sum
*               of all bytes between the two addresses
*
* Arguments:    first_addr is pointer to the initial address of the memory block
*               last_addr is the pointer to the last address of the memory block
**********************************************************************************/
static INT16U CalcCRC_16(INT16U *first_addr, INT16U *last_addr);

/**********************************************************************************
* CalcChkSum() -ascertains checksum for memory block indicated by starting and
*               ending addresses
*
* Description:
*
* Return Value:
*
* Arguments:    startaddr is pointer to the initial address of the memory block
*               endaddr is the pointer to the last address of the memory block
**********************************************************************************/
static INT16U CalcChkSum(INT8U *startaddr, INT8U *endaddr);



/**********************************************************************************
* String Constants
**********************************************************************************/

static const INT8C Hello[] = {"Hello User \n\r"};
static const INT8C LowReq[] = { "Please Enter Low Address:  "};
static const INT8C HighReq[] = {"Please Enter High Address: "};

static const INT8C NonHexErr[] = {"Non-Hex characters present \n\r"};
static const INT8C InTooLong[] = {"Input is too long, needs to be 8 hex"
        " characters at most \n\r" };


void main(void){
    INT8U prg_state;
    INT8U invalid_in = 0U;                 /* Bool variable for program flow */
    INT8C userrequestCRC = 'n';  /* Bool variable for request for 16bit CRC? */
    INT32U low_addr;
    INT32U high_addr;
    INT16U chksum;

    K65TWR_BootClock();
    BIOOpen(BIO_BIT_RATE_9600);             /* Initialize Serial Port  */


    /*Initial SIM6 Clock for CRC-16 Module   */
    SIM->SCGC6 |= SIM_SCGC6_CRC(1U);

    /*Initialize register resources for CRC-16 module*/
    CRC0->CTRL  &= ~(CRC_CTRL_TCRC(0x1U));
    CRC0->CTRL  |= CRC_CTRL_TOT(0x2U);
    CRC0->CTRL  |= CRC_CTRL_TOTR(0x2U);
    CRC0->CTRL  |= CRC_CTRL_FXOR(0x1U);

    CRC0->GPOLY &= CRC_GPOLY_LOW(0x0U); /* Set polynomial*/
    CRC0->GPOLY = CRC_GPOLY_LOW(GPOLYNOMIAL);


    CRC0->CTRL  |= CRC_CTRL_WAS(0x1U); /* Set Seed to 0x0000 */
    CRC0->DATA  &= CRC_DATAL_DATAL(0x0U);
    CRC0->CTRL  &= CRC_CTRL_CLR_WAS;

    /* Display starting message to user, with request for lower address*/
    BIOOutCRLF();
    BIOPutStrg(Hello);
    BIOPutStrg(LowReq);
    prg_state = GET_LOW_ADDR;

    while(1U){
        switch(prg_state){        /* Check for state of Program */

        case(GET_LOW_ADDR):         /* Obtain Low Address entry from
                                     user then check for valid form*/
            invalid_in = CheckUserInput(&low_addr);
            if (invalid_in){
                BIOPutStrg(LowReq);
                prg_state = GET_LOW_ADDR;
            }else{
                prg_state = GET_HIGH_ADDR;
            }
            break;

        case(GET_HIGH_ADDR):
            BIOPutStrg(HighReq);
            /* Obtain High Address entry from user then check
             * for valid form*/
            invalid_in = CheckUserInput(&high_addr);
            if (invalid_in){
                prg_state = GET_HIGH_ADDR;
            }else{
                prg_state = CS_DISPLAY;
                if(high_addr < low_addr){
                    BIOPutStrg("High Address must be greater than "
                            "or equal to the Low Address \n\r");
                    BIOPutStrg(LowReq);
                    prg_state = GET_LOW_ADDR;
                }else{}
            }
            break;

        case(CS_DISPLAY):
            /* Calculate the check sum of all bytes between the two
             * addresses */
            chksum = CalcChkSum( ((void *)low_addr),((void *)high_addr) );	//>>>Type mismatch, not void INT8U* -1bp

            /* Output Low Address, High Address, and check sum to terminal
             * in the form LLLLLLLL-HHHHHHHH XXXX where, LLLLLLLL
             * is the low address HHHHHHHH is the high address and
             *  XXXX is the check sum */
            BIOPutStrg("CS : ");
            BIOOutHexWord(low_addr);
            BIOPutStrg("-");
            BIOOutHexWord(high_addr);
            BIOPutStrg(" ");
            BIOOutHexHWord(chksum);
            BIOOutCRLF();

            /*Check to see if user specified memory block is even*/
            if ( 0 || ((high_addr - low_addr + 1)%2) ){
            } else {
                /* If user types y, CRC-16bit will be generated,
                 * otherwise do nothing */
                BIOPutStrg("Would you like a 16 bit CRC? (y or n)");
                userrequestCRC = BIOGetChar();
                BIOWrite(userrequestCRC);
                BIOOutCRLF();

                if (userrequestCRC == 'y'){
                    /* Output to screen the CRC-16 LLLLLLLL-HHHHHHHH XXXX
                     * where LLLLLLLL is the low address HHHHHHHH is
                     * the high address and XXXX is the CRC */
                    chksum = CalcCRC_16( ((void *)low_addr), ((void *)high_addr) );
                    BIOPutStrg("CRC-16 : ");
                    BIOOutHexWord(low_addr);
                    BIOPutStrg("-");
                    BIOOutHexWord(high_addr);
                    BIOPutStrg(" ");
                    BIOOutHexHWord(chksum);
                    BIOOutCRLF();
                } else {}
            }



            /* Wait till user hits enter to exit CS-Display State*/
            while (BIOGetChar() != '\r'){}
            /* Output new prompt for user*/
            BIOOutCRLF();
            BIOPutStrg(LowReq);
            prg_state = GET_LOW_ADDR;
            break;

                /* Reset Program in case of unexpected error*/
        default:
            /* Output new prompt for user*/
            BIOOutCRLF();
            BIOPutStrg(Hello);
            BIOPutStrg(LowReq);
            prg_state = GET_LOW_ADDR;
            break;
        }

    }
}


/**********************************************************************************
* CalcChkSum() -ascertains checksum for memory block indicated by starting and
*               ending addresses
*
* Description:  Loads every byte from starting address to ending address
*               and takes the first 16 bits of the sum of all said bytes.
*               Note that it if starting address is greater than ending address
*               function will only return byte stored at ending address.
*
* Return Value: c_sum, 16 bit unsigned integer, the first 16 bits of the sum
*               of all bytes between the two addresses
*
* Arguments:    startaddr is pointer to the initial address of the memory block
*               endaddr is the pointer to the last address of the memory block
**********************************************************************************/
static INT16U CalcChkSum(INT8U *startaddr, INT8U *endaddr){
    INT16U c_sum = 0;
    INT8U membyte;
    INT8U *addr = startaddr;

    /*Iterate over all memory locations from start address to address
     * right before ending address and sum up all bytes*/
    while(addr < endaddr){
        membyte = *addr;
        addr++;
        c_sum += (INT16U)membyte;
    }
    c_sum += ( (INT16U)(*endaddr) );

    return c_sum;
}


/**********************************************************************************
* CalcCRC_16() -ascertains checksum for memory block indicated by starting and
*               ending addresses
*
* Description:  Loads every byte from starting address to ending address
*               and calculates the CRC for that memory block
*
* Return Value: crc, 16 bit unsigned integer, the first 16 bits of the sum
*               of all bytes between the two addresses
*
* Arguments:    first_addr is pointer to the initial address of the memory block
*               last_addr is the pointer to the last address of the memory block
**********************************************************************************/
static INT16U CalcCRC_16(INT16U *first_addr, INT16U *last_addr){
    INT16U crc = 0;

    INT16U *addr = first_addr;
    INT16U memshort1 = *addr;
    INT8U toolow = (addr < last_addr);

    INT8U toolow2 = ((addr++) < last_addr) ;
    INT16U memshort2 = *addr;

    INT32U datawrite = 0;

    /*Iterate over all memory locations from start address to address
     * right before ending address and sum up all bytes*/
    while(toolow){

        if (toolow && toolow2){
                    datawrite = ( ((INT32U)(memshort2))<<(16U) | (INT32U)(memshort1) );
                    CRC0->DATA = datawrite;

        } else if (toolow && !toolow2){
                    datawrite = ( 0xFFFFU & (INT32U)(memshort1) );
                    CRC0->DATA = datawrite;
        } else {}

        memshort1 = *addr;
        addr++;
        toolow = (addr < last_addr);

        memshort2 = *addr;
        addr++;
        toolow2 = (addr < last_addr);

    }
    crc = (CRC0->ACCESS16BIT.DATAH) ^ LOW_SHORT;
    /* Reseed the CRC*/
    CRC0->CTRL  |= CRC_CTRL_WAS(0x1U); /* Reset Seed to 0x0000 */
    CRC0->DATA  &= CRC_DATAL_DATAL(0x0U);
    CRC0->CTRL  &= CRC_CTRL_CLR_WAS;


    return crc;
}


/**********************************************************************************
* CheckUserInput() - verifies if user gave valid 32 bit Hex String for address
*
* Description:  A routine that obtains a string input from user and checks
*               the return values of BIOGetString and BIOHexStrgtoWord
*               to see if user gave valid input. For input to be valid, form
*               must be XXXXXXXX, where each character is a hex value. The
*               32bit pointer will be changed.  Only allow user 8 characters,
*               if user enters more terminate user input and request new input
*
* Return Value: 0       -> No error detected
*               1       -> Error
*
* Arguments:    new_addr is a pointer to the 32 bit unsigned integer
*               that will contain the hex value passed by user if valid.
**********************************************************************************/
static INT8U CheckUserInput(INT32U *new_addr){
    INT8U error_code;
    INT8C user_in[USER_IN_LN];
    INT8U wrong_len = BIOGetStrg(USER_IN_LN, user_in);
    INT8U bad_strg = BIOHexStrgtoWord(user_in, new_addr);

    error_code = (wrong_len<<2)|bad_strg; /* Generate Error Code for checking*/

    switch(error_code){       /*Detect which errors if any have been
                                made by user. Send error messages as needed*/
    case(NO_ERROR):
            break;
    case(NON_HEX_CHAR):
            BIOPutStrg(NonHexErr);
            break;
    case(NO_INPUT):
            BIOPutStrg("No input detectable \n\r");
            break;
    case(INPUT_TOO_LONG1):
            BIOPutStrg(InTooLong);
            break;
    case(INPUT_TOO_LONG2):
            BIOPutStrg(InTooLong);
            break;
    case(NON_HEX_AND_TOO_LONG):
            /* Just send error message for input too long*/
            BIOPutStrg(InTooLong);
            break;

    default:
            break;
    }
    return (0 ||error_code);
}
//>>>CRC +3ec
