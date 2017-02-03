#!/usr/bin/env python
# Portions Copyright (C) 2001 Artifex Software Inc.
#   
#   This software is distributed under license and may not be copied, modified
#   or distributed except as expressly authorized under the terms of that
#   license.  Refer to licensing information at http://www.artifex.com/ or
#   contact Artifex Software, Inc., 101 Lucas Valley Road #110,
#   San Rafael, CA  94903, (415)492-9861, for further information. 
#
# TODO
# array data should be wrapped.
#
# text data should be printed as a string not an array of ascii values.
#
# enumerations should printed we now print the ordinal value of the enumeration.
#
# make self.unpack endian like with binding

# DIFFS between HP
# Artifex reports the file offset of each operator HP does not.

# for packing and unpacking binary data
from __future__ import print_function
import sys
if sys.version < '3':
    def chr_(x):
        return x
    sys_stdout_write = sys.stdout.write
else:
    def chr_(x):
        return chr(x)
    sys_stdout_write = sys.stdout.buffer.write

import re
from struct import *
import string 
import sys

DEBUG = 0

# Workaround stupid windows stdout not being binary safe
if sys.platform == "win32":
    import os, msvcrt
    msvcrt.setmode(sys.stdout.fileno(), os.O_BINARY)

# tags
pxl_tags_dict = {                                  
    'ArcPath' :                 0x91,                  
    'BeginChar' :               0x52,
    'BeginFontHeader' :         0x4f,
    'BeginImage' :              0xb0,
    'BeginPage' :               0x43,
    'BeginRastPattern' :        0xb3,
    'BeginScan' :               0xb6,
    'BeginSession' :            0x41,
    'BeginStream' :             0x5b,
    'BeginUserDefinedLineCap' : 0x82,
    'BezierPath' :              0x93,
    'BezierRelPath' :           0x95,
    'Chord' :                   0x96,
    'ChordPath' :               0x97,
    'CloseDataSource' :         0x49,
    'CloseSubPath' :            0x84,
    'Comment' :                 0x47,
    'Ellipse' :                 0x98,
    'EllipsePath' :             0x99,
    'EndChar' :                 0x54,
    'EndFontHeader' :           0x51,
    'EndImage' :                0xb2,
    'EndPage' :                 0x44,
    'EndRastPattern' :          0xb5,
    'EndScan' :                 0xb8,
    'EndSession' :              0x42,
    'EndStream' :               0x5d,
    'EndUserDefinedLineCaps' :  0x83,
    'ExecStream' :              0x5e,
    'LinePath' :                0x9b,
    'LineRelPath' :             0x9d,
    'NewPath' :                 0x85,
    'OpenDataSource' :          0x48,
    'PaintPath' :               0x86,
    'Passthrough' :             0xbf,
    'Pie' :                     0x9e,
    'PiePath' :                 0x9f,
    'PopGS' :                   0x60,
    'PushGS' :                  0x61,
    'ReadChar' :                0x53,
    'ReadFontHeader' :          0x50,
    'ReadImage' :               0xb1,
    'ReadRastPattern' :         0xb4,
    'ReadStream' :              0x5c,
    'Rectangle' :               0xa0,
    'RectanglePath' :           0xa1,
    'RemoveFont' :              0x55,
    'RemoveStream' :            0x5f,
    'RoundRectangle' :          0xa2,
    'RoundRectanglePath' :      0xa3,
    'ScanLineRel' :             0xb9,
    'SetAdaptiveHalftoning' :   0x94,
    'SetBrushSource' :          0x63,
    'SetCharAttributes' :       0x56,
    'SetCharAngle' :            0x64,
    'SetCharBoldValue' :        0x7d,
    'SetCharScale' :            0x65,
    'SetCharShear' :            0x66,
    'SetCharSubMode' :          0x81,
    'SetClipIntersect' :        0x67,
    'SetClipMode' :             0x7f,
    'SetClipRectangle' :        0x68,
    'SetClipReplace' :          0x62,
    'SetClipToPage' :           0x69,
    'SetColorSpace' :           0x6a,
    'SetColorTrapping' :        0x92,
    'SetColorTreatment' :       0x58,
    'SetCursor' :               0x6b,
    'SetCursorRel' :            0x6c,
    'SetDefaultGS' :            0x57,
    'SetHalftoneMethod' :       0x6d,
    'SetFillMode' :             0x6e,
    'SetFont' :                 0x6f,
    'SetHalftoneMethod' :       0x6d,
    'SetLineCap' :              0x71,
    'SetLineDash' :             0x70,
    'SetLineJoin' :             0x72,
    'SetMiterLimit' :           0x73,
    'SetNeutralAxis' :          0x7e,
    'SetPageDefaultCTM' :       0x74,
    'SetPageOrigin' :           0x75,
    'SetPageRotation' :         0x76,
    'SetPageScale' :            0x77,
    'SetPathToClip' :           0x80,
    'SetPatternTxMode' :        0x78,
    'SetPenSource' :            0x79,
    'SetPenWidth' :             0x7a,
    'SetROP' :                  0x7b,
    'SetSourceTxMode' :         0x7c,
    'Text' :                    0xa8,
    'TextPath' :                0xa9,
    'VendorUnique' :            0x46,
    'attr_ubyte' :              0xf8,
    'attr_uint16' :             0xf9,
    'embedded_data' :           0xfa,
    'embedded_data_byte' :      0xfb,
    'real32' :                  0xc5,
    'real32_array' :            0xcd,
    'real32_box' :              0xe5,
    'real32_xy' :               0xd5,
    'sint16' :                  0xc3,
    'sint16_array' :            0xcb,
    'sint16_box' :              0xe3,
    'sint16_xy' :               0xd3,
    'sint32' :                  0xc4,
    'sint32_array' :            0xcc,
    'sint32_box' :              0xe4,
    'sint32_xy' :               0xd4,
    'ubyte' :                   0xc0,
    'ubyte_array' :             0xc8,
    'ubyte_box' :               0xe0,
    'ubyte_xy' :                0xd0,
    'uint16' :                  0xc1,
    'uint16_array' :            0xc9,
    'uint16_box' :              0xe1,
    'uint16_xy' :               0xd1,
    'uint32' :                  0xc2,
    'uint32_array' :            0xca,
    'uint32_box' :              0xe2,
    'uint32_xy' :               0xd2
}

pxl_enumerations_dict = {
    'ArcDirection' : [ 'eClockWise=0',  'eCounterClockWise=1' ],
    'BackCh' : ['eErrorPage=0'], # deprecated.
    'CharSubModeArray' : [ 'eNoSubstitution=0', 'eVerticalSubstitution=1' ],
    'ClipMode' : ['eNonZeroWinding=0', 'eEvenOdd=1' ],
    'ClipRegion' : ['eInterior=0', 'eExterior=1'],
    'ColorDepth' : [ 'e1Bit=0', 'e4Bit=1', 'e8Bit=2' ],
    'ColorMapping' : [ 'eDirectPixel=0', 'eIndexedPixel=1' ],
    'ColorSpace' : [ 'eGray=1', 'eRGB=2', 'eSRGB=3' ], # srgb deprecated
    'ColorTreatment' : [ 'eNoTreatment=0', 'eScreenMatch=1', 'eVivid=2' ],
    'CompressMode' : [ 'eNoCompression=0', 'eRLECompression=1',
                       'eJPEGCompression=2', 'eDeltaRowCompression=3' ],
    'DataOrg' : [ 'eBinaryHighByteFirst=0', 'eBinaryLowByteFirst=1' ],
    'DataSource' : [ 'eDefault=0' ],
    'DataType' : [ 'eUByte=0', 'eSByte=1', 'eUint16=2', 'eSint16=3' ],
    'DitherMatrix' : [ 'eDeviceBest=0' ],
    'DuplexPageMode' : [ 'eDuplexHorizontalBinding=0', 'eDuplexVerticalBinding=1' ],
    'DuplexPageSide' : [ 'eFrontMediaSide=0', 'eBackMediaSide=1' ],
    'ErrorReport' : ['eNoReporting=0', 'eBackChannel=1', 'eErrorPage=2',
                     'eBackChAndErrPage=3', 'eNWBackChannel=4', 'eNWErrorPage=5',
                     'eNWBackChAndErrPage=6' ],
    'FillMode' : ['eNonZeroWinding=0', 'eEvenOdd=1' ],
    'LineJoineMiterJoin' : [ 'eRoundJoin=0', 'eBevelJoin=1', 'eNoJoin=2' ],
    'MediaSource' : [ 'eDefaultSource=0', 'eAutoSelect=1', 'eManualFeed=2',
                      'eMultiPurposeTray=3', 'eUpperCassette=4', 'eLowerCassette=5',
                      'eEnvelopeTray=6', 'eThirdCassette=7', 'External Trays=8-255' ],
    'MediaDestination' :  [ 'eDefaultDestination=0', 'eFaceDownBin=1', 'eFaceUpBin=2',
                            'eJobOffsetBin=3', 'External Bins=5-255' ],
    'LineCapStyle' : [ 'eButtCap=0' 'eRoundCap=1', 'eSquareCap=2', 'eTriangleCap=3' ],
    'LineJoin' : [ 'eMiterJoin=0', 'eRoundJoin=1', 'eBevelJoin=2', 'eNoJoin=3' ],
    'Measure' : [ 'eInch=0', 'eMillimeter=1', 'eTenthsOfAMillimeter=2' ],
    'MediaSize' : [ 'eDefault = 96', 'eLetterPaper=0', 'eLegalPaper=1', 'eA4Paper=2',
                    'eExecPaper=3', 'eLedgerPaper=4', 'eA3Paper=5',
                    'eCOM10Envelope=6', 'eMonarchEnvelope=7', 'eC5Envelope=8',
                    'eDLEnvelope=9', 'eJB4Paper=10', 'eJB5Paper=11', 'eB5Paper=13',
                    'eB5Envelope=12', 'eJPostcard=14', 'eJDoublePostcard=15',
                    'eA5Paper=16', 'eA6Paper=17', 'eJBPaper=18', 'JIS8K=19',
                    'JIS16K=20', 'JISExec=21' ],
    'Orientation' : ['ePortraitOrientation=0', 'eLandscapeOrientation=1',
                     'eReversePortrait=2', 'eReverseLandscape=3',
                     'eDefaultOrientation=4' ],
    'PatternPersistence' : [ 'eTempPattern=0', 'ePagePattern=1', 'eSessionPattern=2'],
    'SimplexPageMode' : ['eSimplexFrontSide=0'],
    'TxMode' : [ 'eOpaque=0', 'eTransparent=1' ],
    'WritingMode' : [ 'eHorizontal=0', 'eVertical=1' ]
}

pxl_attribute_name_to_attribute_number_dict = { 
    'AllObjectTypes' : 29,
    'ArcDirection' : 65,
    'BlockByteLength' : 111,
    'BlockHeight' : 99,
    'BoundingBox' : 66,
    'ColorimetricColorSpace': 17, # deprecated
    'CharAngle' : 161,
    'CharBoldValue' : 177,
    'CharCode' : 162,
    'CharDataSize' : 163,
    'CharScale' : 164,
    'CharShear' : 165,
    'CharSize' : 166,
    'CharSubModeArray' : 172,
    'ClipMode' : 84,
    'ClipRegion' : 83,
    'ColorDepth' : 98,
    'ColorMapping' : 100,
    'ColorSpace' : 3,
    'ColorTreatment' : 120,
    'CommentData' : 129,
    'CompressMode' : 101,
    'ControlPoint1' : 81,
    'ControlPoint2' : 82,
    'CRGBMinMax' : 20, # deprecated
    'CustomMediaSize' : 47,
    'CustomMediaSizeUnits' : 48,
    'DashOffset' : 67,
    'DataOrg' : 130,
    'DestinationBox' : 102,
    'DestinationSize' : 103,
    'DeviceMatrix' : 33,
    'DitherMatrixDataType' : 34,
    'DitherMatrixDepth' : 51,
    'DitherMatrixSize' : 50,
    'DitherOrigin' : 35,
    'DuplexPageMode' : 53,
    'DuplexPageSide' : 54,
    'EllipseDimension' : 68,
    'EndPoint' : 69,
    'ErrorReport' : 143,
    'FillMode' : 70,
    'FontFormat' : 169,
    'FontHeaderLength' : 167,
    'FontName' : 168,
    'GammaGain' : 21, # deprecated.
    'GrayLevel' : 9,
    'LineCapStyle' : 71,
    'LineDashStyle' : 74,
    'LineJoinStyle' : 72,
    'Measure' : 134,
    'MediaDestination' : 36,
    'MediaSize' : 37,
    'MediaSource' : 38,
    'MediaType' : 39,
    'MiterLength' : 73,
    'NewDestinationSize' : 13,
    'NullBrush' : 4,
    'NullPen' : 5,
    'NumberOfPoints' : 77,
    'NumberOfScanLines' : 115,
    'Orientation' : 40,
    'PCLSelectFont' : 141,
    'PadBytesMultiple' : 110,
    'PageAngle' : 41,
    'PageCopies' : 49,
    'PageOrigin' : 42,
    'PageScale' : 43,
    'PaletteData' : 6,
    'PaletteDepth' : 2,
    'PatternDefineID' : 105,
    'PatternOrigin' : 12,
    'PatternPersistence' : 104,
    'PatternSelectID' : 8,
    'PenWidth' : 75,
    'Point' : 76,
    'PointType' : 80,
    'PrimaryArray' : 14,
    'PrimaryDepth' : 15,
    'PrintableArea' : 116,
    'RGBColor' : 11,
    'ROP3' : 44,
    'RasterObjects' : 32,
    'SimplexPageMode' : 52,
    'SolidLine' : 78,
    'SourceHeight' : 107,
    'SourceType' : 136,
    'SourceWidth' : 108,
    'StartLine' : 109,
    'StartPoint' : 79,
    'StreamDataLength' : 140,
    'StreamName' : 139,
    'SymbolSet' : 170,
    'TextData' : 171,
    'TextObjects' : 30,
    'TxMode' : 45,
    'UnitsPerMeasure' : 137,
    'VectorObjects' : 31,
    'VUExtension' : 145,
    'VUDataLength' : 146,
    'VUAttr1' : 147,
    'VUAttr2' : 148,
    'VUAttr3' : 149,
    'VUAttr4' : 150,
    'VUAttr5' : 151,
    'VUAttr6' : 152,
    'WhiteReferencePoint' : 19, # deprecated.
    'WritingMode' : 173, # deprecated.
    'XSpacingData' : 175, 
    'XYChromaticities' : 18, # deprecated.
    'YSpacingData' : 176,
}

class pxl_asm:

    def __init__(self, data):
        # ` HP-PCL XL;3;0
        index = data.index(b"` HP-PCL XL;")
        data = data[index:]
        self.data = data
                # parse out data order and protocol
        self.binding = chr_(data[0])
        self.protocol = chr_(data[12])
        self.revision = chr_(data[14])

        # pointer to data
        self.index = 0
        # NB this screws up file indexing - remove all comments
        self.data = re.sub( b'\/\/.*\n', b'', self.data )

        # print out big endian protocol and revision.  NB should check
        # revisions are the same.
        print("\033%-12345X@PJL ENTER LANGUAGE = PCLXL")
        print(") HP-PCL XL;" + self.protocol + ";" + self.revision)
        sys.stdout.flush()

        # skip over protocol and revision
        while( chr_(data[self.index]) != '\n' ):
            self.index = self.index + 1
        self.index = self.index + 1

        # saved size of last array parsed
        self.size_of_array = -1;
        self.pack_string = ""

        # dictionary of streams keyed by stream name
        self.user_defined_streams = {}

        # the n'th operator in the stream
        self.operator_position = 0
        self.__verbose = DEBUG

        # the file must be ascii encode to assemble.
        if (self.binding != '`'):
            raise(SyntaxError)

        # output is always little endian.
        self.assembled_binding = '<'
        
    def nullAttributeList(self):
        return 0

    # does not consume the string
    def next_string(self):
        index = self.index
        while chr_(self.data[index]) in string.whitespace: index = index + 1
        start = index
        while chr_(self.data[index]) not in string.whitespace: index = index + 1
        end = index
        return self.data[start:end].decode()

    def consume_next_string(self):
        while chr_(self.data[self.index]) in string.whitespace:
            self.index = self.index + 1
        while chr_(self.data[self.index]) not in string.whitespace:
            self.index = self.index + 1

    # redefine pack to handle endianness
    def pack(self, format, *data):
        for args in data:
            try:
                sys_stdout_write(pack(self.assembled_binding + format, args))
            except:
                sys.stderr.write("assemble failed at: ")
                # dump surrounding context.
                sys.stderr.write(self.data[self.index:self.index+40].decode())
                sys.stderr.write("\n")
                raise
    # implicitly read when parsing the tag
    def attributeIDValue(self):
        return 1

    # search for next expected tag "tag" and print its hex value.
    def getTag(self, tag):
        new_tag = self.next_string()
        if ( new_tag == tag ):
            self.consume_next_string()
            self.pack( "B", pxl_tags_dict[tag] )
            return 1

        return 0

    # get the next operator
    def operatorTag(self):
        tag = self.next_string()
        if ( not self.is_Embedded(tag) ):
            self.operator_position = self.operator_position + 1
        if ( tag in pxl_tags_dict ):
            self.pack( 'B', pxl_tags_dict[tag] )
            self.consume_next_string()
            # handle special cases
            if ( self.is_Embedded(tag) ):
                self.process_EmbeddedInfo(tag)
            if ( tag == 'VendorUnique' ):
                self.process_VUpayload()
            return 1
        return 0

    def Tag_ubyte(self):
        if ( self.getTag( 'ubyte' ) ):
             self.pack_string = 'B'
             return 1
        return 0

    def Tag_sint16(self):
        if ( self.getTag( 'sint16' ) ):
            self.pack_string = 'h'
            return 1
        return 0

    def Tag_uint16(self):
        if ( self.getTag( 'uint16' ) ):
             self.pack_string = 'H'
             return 1
        return 0

    def Tag_sint32(self):
        if ( self.getTag( 'sint32' ) ):
            self.pack_string = 'l'
            return 1
        return 0

    def Tag_uint32(self):
        if ( self.getTag( 'uint32' ) ):
             self.pack_string = 'L'
             return 1
        return 0

    def Tag_real32(self):
        if ( self.getTag( 'real32' ) ):
            self.pack_string = 'f'
            return 1
        return 0

    def consume_to_char_plus_one(self, inchr):
        while (self.data[self.index:self.index+len(inchr)].decode() != inchr):
            self.index = self.index + 1
        self.index = self.index + len(inchr)
            
    def Tag_ubyte_array(self):
        if ( self.getTag( 'ubyte_array' ) ):
            self.consume_to_char_plus_one('[')
            self.pack_string = 'B'
            return 1
        return 0

    def Tag_uint16_array(self):
        if ( self.getTag( 'uint16_array' ) ):
            self.pack_string = 'H'
            self.consume_to_char_plus_one('[')
            return 1
        return 0

    def Tag_sint16_array(self):
        if ( self.getTag( 'sint16_array' ) ):
            self.pack_string = 'h'
            self.consume_to_char_plus_one('[')
            return 1
        return 0

    def Tag_uint32_array(self):
        if ( self.getTag( 'uint32_array' ) ):
            self.pack_string = 'L'
            self.consume_to_char_plus_one('[')
            return 1
        return 0

    def Tag_sint32_array(self):
        if ( self.getTag( 'sint32_array' ) ):
            self.pack_string = 'l'
            self.consume_to_char_plus_one('[')
            return 1
        return 0

    def Tag_real32_array(self):
        if ( self.getTag( 'real32_array' ) ):
            self.pack_string = 'f'
            self.consume_to_char_plus_one('[')
            return 1
        return 0
    
    def Tag_ubyte_xy(self):
        if ( self.getTag( 'ubyte_xy' ) ):
            self.pack('B', self.next_num(), self.next_num())
            return 1
        return 0

    def Tag_uint16_xy(self):
        if ( self.getTag( 'uint16_xy' ) ):
            self.pack('H', self.next_num(), self.next_num())
            return 1
        return 0

    def Tag_sint16_xy(self):
        if ( self.getTag( 'sint16_xy' ) ):
            self.pack('h', self.next_num(), self.next_num())
            return 1
        return 0

    def Tag_uint32_xy(self):
        if ( self.getTag( 'uint32_xy' ) ):
            self.pack('L', self.next_num(), self.next_num())
            return 1
        return 0

    def Tag_sint32_xy(self):
        if ( self.getTag( 'sint32_xy' ) ):
            self.pack('l', self.next_num(), self.next_num())
            return 1
        return 0

    def Tag_real32_xy(self):
        if ( self.getTag( 'real32_xy' ) ):
            self.pack('f', self.next_num(), self.next_num())
            return 1
        return 0
    
    def Tag_ubyte_box(self):
        if ( self.getTag( 'ubyte_box' ) ):
            self.pack('B', self.next_num(), self.next_num(),
                      self.next_num(), self.next_num())
            return 1
        return 0

    def Tag_uint16_box(self):
        if ( self.getTag( 'uint16_box' ) ):
            self.pack('H', self.next_num(), self.next_num(),
                      self.next_num(), self.next_num())
            return 1
        return 0

    def Tag_sint16_box(self):
        if ( self.getTag( 'sint16_box' ) ):
            self.pack('h', self.next_num(), self.next_num(),
                      self.next_num(), self.next_num())
            return 1
        return 0

    def Tag_uint32_box(self):
        if ( self.getTag( 'uint32_box' ) ):
            self.pack('L', self.next_num(), self.next_num(),
                      self.next_num(), self.next_num())
            return 1
        return 0

    def Tag_sint32_box(self):
        if ( self.getTag( 'sint32_box' ) ):
            self.pack('l', self.next_num(), self.next_num(),
                      self.next_num(), self.next_num())
            return 1
        return 0

    def Tag_real32_box(self):
        if ( self.getTag( 'real32_box' ) ):
            self.pack('f', self.next_num(), self.next_num(),
                      self.next_num(), self.next_num())
            return 1
        return 0
    
    # check for embedded tags.
    def is_Embedded(self, name):
        return ( name == 'embedded_data' or name == 'embedded_data_byte' )

    def process_EmbeddedInfo(self, name):
        # skip over the
        # finally write the list
        self.consume_to_char_plus_one( '[' )
        number_list = []
        while (1):
            num = self.next_hex_num()
            # trick - num will fail on ']'
            if (num != None):
                number_list.append(num)
            else:
                break
        # write the length of the list as the embedded data's size
        if ( name == 'embedded_data' ):
            format = 'L'
        else:
            format = 'B'
        self.pack(format, len(number_list))
        # NB needs wrapping
        for num in number_list:
            self.pack( 'B', num )

    # VU payload does not have a prepended length 
    # and is simplier than embedded data
    def process_VUpayload(self):
        self.consume_to_char_plus_one( '[' )
        while (1):
            num = self.next_hex_num()
            # trick - num will fail on ']'
            if (num != None):
                self.pack( 'B', num )
            else:
                break

    def Tag_attr_ubyte(self):
        tag = self.next_string()
        if ( tag in pxl_attribute_name_to_attribute_number_dict ):
            self.pack( 'B', pxl_tags_dict['attr_ubyte'] )
            self.pack( 'B', pxl_attribute_name_to_attribute_number_dict[tag] )
            self.consume_next_string()
            # handle special cases
            if ( self.is_Embedded(tag) ):
                self.process_EmbeddedInfo(tag)
            return 1
        else:
            print("Unlisted attribute tag:", tag, file=sys.stderr)
            raise(SyntaxError)("Unlisted attribute tag found in PXL disassembly")
        return 0

    def Tag_attr_uint16(self):
        if ( self.getTag( 'attr_uint16' ) ):
            print("Attribute tag uint16 # NOT IMPLEMENTED #", self.pack('HH', self.data[self.index] ))
            self.index = self.index + 2
            return 1
        return 0

    def attributeID(self):
        return (self.Tag_attr_ubyte() or self.Tag_attr_uint16()) and self.attributeIDValue()

    # return the start and end position of the next string
    def next_token(self):
        # token begins or end on a line
        while chr_(self.data[self.index]) in string.whitespace:
            self.index = self.index + 1
        start = self.index
        while chr_(self.data[self.index]) not in string.whitespace:
            self.index = self.index + 1
        end = self.index
        pos = start
        # return offset within the line of the start of the token.
        # Useful for assembling hex format.
        while (chr_(self.data[pos]) != '\n'):
            if pos == 0:
                break
            pos -= 1
        return self.data[start:end].decode(), start-pos-1
    
    def next_hex_num(self):
        num_str, offset = self.next_token()
        # end of data
        if ( offset == 0 and num_str == ']' ):
            return None

        # offset or ascii columns
        if ( offset < 7 or offset > 57 ):
            return self.next_hex_num()

        # hex number
        return int(num_str, 16)
        
    def next_num(self):
        # no checking.
        num_str, offset = self.next_token()
        try:
            num = int(num_str)
        except ValueError:
            try:
                num = float(num_str)
            except:
                num = None
        return num
    
    def singleValueType(self):
        if ( self.Tag_ubyte() or self.Tag_uint16() or self.Tag_uint32() or \
             self.Tag_sint16() or self.Tag_sint32() or self.Tag_real32() ):
            self.pack(self.pack_string, self.next_num()),
            return 1
        return 0

    def xyValueType(self):
        return self.Tag_ubyte_xy() or self.Tag_uint16_xy() or self.Tag_uint32_xy() or \
               self.Tag_sint16_xy() or self.Tag_sint32_xy() or self.Tag_real32_xy()
        
    def boxValueType(self):
        return self.Tag_ubyte_box() or self.Tag_uint16_box() or self.Tag_uint32_box() or \
               self.Tag_sint16_box() or self.Tag_sint32_box() or self.Tag_real32_box()
        
    def valueType(self):
        return self.singleValueType() or self.xyValueType() or self.boxValueType()

    # don't confuse the size of the type with the size of the elements
    # in the array
    def arraySizeType(self):
        return (self.Tag_ubyte() or self.Tag_uint16())

    def arraySize(self):
        # save the old pack string for the type of the array, the data
        # type for the size will replace it.
        pack_string = self.pack_string
        if ( self.arraySizeType() ):
            self.size_of_array = self.next_num()
            self.pack(self.pack_string, self.size_of_array)
            # restore the pack string
            self.pack_string = pack_string
            return 1
        return 0
        
    def singleValueArrayType(self):
        return self.Tag_ubyte_array() or self.Tag_uint16_array() or \
               self.Tag_uint32_array() or self.Tag_sint16_array() or \
               self.Tag_sint32_array() or self.Tag_real32_array()
        
    def arrayType(self):
        if (self.singleValueArrayType() and self.arraySize()):
            hex_dump_format = (self.pack_string == 'B')
            for num in range(0, self.size_of_array):
                # reading byte data hex dump format
                if hex_dump_format:
                    n = self.next_hex_num()
                # not hex dump format
                else:
                    n = self.next_num()
                self.pack(self.pack_string, n)
            if hex_dump_format:
                self.consume_to_char_plus_one('\n]')
            else:
                self.consume_to_char_plus_one(']')
            return 1
        return 0

    def dataType(self):
        return( self.valueType() or self.arrayType() or self.boxValueType() )

    # these get parsed when doing the tags
    def numericValue(self):
        return 1;

    def attributeValue(self):
        return( self.dataType() and self.numericValue() )

    def singleAttributePair(self):
        return( self.attributeValue() and self.attributeID() )
    
    def multiAttributeList(self):
        # NB should be many 1+ not sure how this get handled yet
        return( self.singleAttributePair() )
    
    def nullAttributeList(self):
        return 0
    
    def attributeList(self):
        return (self.singleAttributePair() or self.multiAttributeList() or self.nullAttributeList())

    def attributeLists(self):
        # save the beginning of the attribute list even if it is
        # empty.  So we can report the position of the command.
        self.begin_attribute_pos = self.index
        # 0 or more attribute lists
        while( self.attributeList() ):
            continue
        return 1

    def UEL(self):
        uel_string_1 = 'string*'
        uel_string_2 = b'-12345X'
        tag = self.next_string()
        if ( tag == uel_string_1 ):
            self.consume_next_string()
            # an approximate search
            if (self.data[self.index:].find( uel_string_2 ) >= 0 ):
                self.consume_to_char_plus_one('X')
                sys.stdout.write( "\033%-12345X" )
                sys.stdout.flush()
                return 1
        return 0
        
    def operatorSequences(self):
        while ( self.attributeLists() and self.operatorTag() ) or self.UEL():
            continue
        
    def assemble(self):
        try:
            self.operatorSequences()
        # assume an index error means we have processed everything - ugly
        except IndexError:
             return
        else:
            sys.stderr.write("assemble failed\n")

if __name__ == '__main__':
    import sys

    if not sys.argv[1:]:
        print("Usage: %s pxl files" % sys.argv[0])
        
    for file in sys.argv[1:]:
        try:
            fp = open(file, 'rb')
        except:
            sys.stderr.write("Cannot find file %s" % file)
            continue
        # read the whole damn thing.  Removing comments and blank lines.
        pxl_code = fp.read()
        fp.close()

        # initialize and assemble.
        pxl_stream = pxl_asm(pxl_code)
        pxl_stream.assemble()
