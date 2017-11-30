#ifndef __PIN_MAP_H__
#define __PIN_MAP_H__

#define GATEWAY_V2_3CHANNEL

#if defined (GATEWAY_V1)
/*---------------- SX1278-1 ----------------------*/
#define    SX1278_1_SPI_MISO_PIN        42 // GPIO42
#define    SX1278_1_SPI_MOSI_PIN        41 // GPIO41
#define    SX1278_1_SPI_CLK_PIN         40 // GPIO40
#define    SX1278_1_SPI_CS_PIN           6 // GPIO06

#define    SX1278_1_RST_PIN             14 // GPIO14

#define    SX1278_1_DIO0_PIN            18 // GPIO18
#define    SX1278_1_DIO1_PIN            19 // GPIO19
#define    SX1278_1_DIO2_PIN            11 // GPIO11
#define    SX1278_1_DIO3_PIN            16 // GPIO16
#define    SX1278_1_DIO4_PIN            17 // GPIO17
#define    SX1278_1_DIO5_PIN            15 // GPIO15

/*---------------- SX1278-2 ----------------------*/
#define    SX1278_2_SPI_MISO_PIN        42 // GPIO42
#define    SX1278_2_SPI_MOSI_PIN        41 // GPIO41
#define    SX1278_2_SPI_CLK_PIN         40 // GPIO40
#define    SX1278_2_SPI_CS_PIN          39 // GPIO39

#define    SX1278_2_RST_PIN              0 // GPIO00

#define    SX1278_2_DIO0_PIN            29 // GPIO01
#define    SX1278_2_DIO1_PIN             4 // GPIO04
#define    SX1278_2_DIO2_PIN             5 // GPIO05
#define    SX1278_2_DIO3_PIN             3 // GPIO03
#define    SX1278_2_DIO4_PIN            37 // GPIO37
#define    SX1278_2_DIO5_PIN             2 // GPIO02
#elif defined (GATEWAY_V2_2CHANNEL)
/*---------------- SX1278-1 ----------------------*/
#define    SX1278_1_SPI_MISO_PIN        42 // GPIO42
#define    SX1278_1_SPI_MOSI_PIN        41 // GPIO41
#define    SX1278_1_SPI_CLK_PIN         40 // GPIO40
#define    SX1278_1_SPI_CS_PIN           6 // GPIO06

#define    SX1278_1_RST_PIN             14 // GPIO14

#define    SX1278_1_DIO0_PIN            18 // GPIO18
#define    SX1278_1_DIO1_PIN            19 // GPIO19
#define    SX1278_1_DIO2_PIN            11 // GPIO11
#define    SX1278_1_DIO3_PIN            16 // GPIO16
#define    SX1278_1_DIO4_PIN            17 // GPIO17
#define    SX1278_1_DIO5_PIN            15 // GPIO15

/*---------------- SX1278-2 ----------------------*/
#define    SX1278_2_SPI_MISO_PIN        42 // GPIO42
#define    SX1278_2_SPI_MOSI_PIN        41 // GPIO41
#define    SX1278_2_SPI_CLK_PIN         40 // GPIO40
#define    SX1278_2_SPI_CS_PIN          39 // GPIO39

#define    SX1278_2_RST_PIN              0 // GPIO00

#define    SX1278_2_DIO0_PIN            29 // GPIO01
#define    SX1278_2_DIO1_PIN             4 // GPIO04
#define    SX1278_2_DIO2_PIN             5 // GPIO05
#define    SX1278_2_DIO3_PIN             3 // GPIO03
#define    SX1278_2_DIO4_PIN            37 // GPIO37
//#define    SX1278_2_DIO5_PIN             2 // GPIO02
#elif defined (GATEWAY_V2_3CHANNEL)
/*---------------- SX1278-1 ----------------------*/
#define    SX1278_1_SPI_MISO_PIN        42 // GPIO42
#define    SX1278_1_SPI_MOSI_PIN        41 // GPIO41
#define    SX1278_1_SPI_CLK_PIN         40 // GPIO40
#define    SX1278_1_SPI_CS_PIN           6 // GPIO06

#define    SX1278_1_RST_PIN             14 // GPIO14

#define    SX1278_1_DIO0_PIN            18 // GPIO18
#define    SX1278_1_DIO1_PIN            19 // GPIO19
#define    SX1278_1_DIO2_PIN            11 // GPIO11
#define    SX1278_1_DIO3_PIN            16 // GPIO16
#define    SX1278_1_DIO4_PIN            17 // GPIO17
#define    SX1278_1_DIO5_PIN            15 // GPIO15

/*---------------- SX1278-2 ----------------------*/
#define    SX1278_2_SPI_MISO_PIN        42 // GPIO42
#define    SX1278_2_SPI_MOSI_PIN        41 // GPIO41
#define    SX1278_2_SPI_CLK_PIN         40 // GPIO40
#define    SX1278_2_SPI_CS_PIN          39 // GPIO39

#define    SX1278_2_RST_PIN              0 // GPIO00

#define    SX1278_2_DIO0_PIN            29 // GPIO01
#define    SX1278_2_DIO1_PIN             4 // GPIO04
#define    SX1278_2_DIO2_PIN             5 // GPIO05
#define    SX1278_2_DIO3_PIN             3 // GPIO03
#define    SX1278_2_DIO4_PIN            37 // GPIO37
//#define    SX1278_2_DIO5_PIN             2 // GPIO02
/*---------------- SX1278-3 ----------------------*/
#define    SX1278_3_SPI_MISO_PIN        42 // GPIO42
#define    SX1278_3_SPI_MOSI_PIN        41 // GPIO41
#define    SX1278_3_SPI_CLK_PIN         40 // GPIO40
#define    SX1278_3_SPI_CS_PIN          1 // GPIO1

#define    SX1278_3_RST_PIN              27 // GPIO27

#define    SX1278_3_DIO0_PIN            24 // GPIO24
#define    SX1278_3_DIO1_PIN             23 // GPIO23
#define    SX1278_3_DIO2_PIN             22 // GPIO22
#define    SX1278_3_DIO3_PIN             26 // GPIO26
#define    SX1278_3_DIO4_PIN            25 // GPIO25
#else
#error "please define hardware version"
#endif
#endif