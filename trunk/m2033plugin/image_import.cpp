/******************************************************************************
This source file is part of metro2033-tools.
Please visit http://code.google.com/p/metro2033-tools/ for more information.

Copyright (C) 2010 Ivan Shishkin <codingdude@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
******************************************************************************/

#include <stdio.h>
#include <string>
#include "image_import.h"
#include "squish.h"

using namespace m2033;

void image_import::ShowAbout( HWND hwnd )
{
	MessageBox( hwnd,
				"Metro 2033 Image import plugin.\n"
				"Please visit http://code.google.com/p/metro2033-tools/ for more information.\n"
				"Copyright (C) 2010 Ivan Shishkin <codingdude@gmail.com>\n",
				"About",
				MB_ICONINFORMATION );
}

BitmapStorage* image_import::Load( BitmapInfo *bi, Bitmap *map, BMMRES *status )
{
	BitmapStorage *storage;
	FILE* file;
	size_t size, img_size;
	int flags;
	void* buffer;
	squish::u8 *img, *p;
	BMM_Color_64 color[2048];
	BOOL res;
	const TCHAR* fname;

	GetImageInfo( bi );
	fname = bi->Name();
	file = fopen( fname, "rb" );
	if( file == 0 )
	{
		*status = BMMRES_FILENOTFOUND;
		return 0;
	}

	fseek( file, 0, SEEK_END );
	size = ftell( file );
	fseek( file, 0, SEEK_SET );

	switch( size )
	{
	case 174776:
		flags = squish::kDxt1;
		break;
	case 349552:
		flags = squish::kDxt5;
		break;
	case 524288:
		flags = squish::kDxt1;
		break;
	case 1048576:
		flags = squish::kDxt5;
		break;
	case 2097152:
		flags = squish::kDxt1;
		break;
	case 4194304:
		flags = squish::kDxt5;
		break;
	default:
		*status = BMMRES_INVALIDFORMAT;
		return 0;
	}

	buffer = malloc( size );
	fread( buffer, 1, size, file );
	fclose( file );

	img_size = bi->Width() * bi->Height() * 4;
	img = (squish::u8*) malloc( img_size );
	squish::DecompressImage( img, bi->Width(), bi->Height(), buffer, flags );

	free( buffer );

	storage = BMMCreateStorage( map->Manager(), BMM_TRUE_32 );
	if( storage == 0 )
	{
		*status = BMMRES_CANTSTORAGE;
		free( img );
		return 0;
	}

	res = storage->Allocate( bi, map->Manager(), BMM_OPEN_R );
	if( res == 0 )
	{
		*status = BMMRES_MEMORYERROR;
		free( img );
		delete storage;
		return 0;
	}

	p = img;

	for( int i = 0; i < bi->Width(); i++ )
	{
		for( int j = 0; j < bi->Height(); j++ )
		{
			color[j].r = (WORD)((*p++) << 8);
			color[j].g = (WORD)((*p++) << 8);
			color[j].b = (WORD)((*p++) << 8);
			color[j].a = (WORD)((*p++) << 8);
		}

		res = storage->PutPixels( 0, i, bi->Width(), color );
		if( res == 0 )
		{
			*status = BMMRES_MEMORYERROR;
			free( img );
			delete storage;
			return 0;
		}
	}

	free( img );

	storage->bi.CopyImageInfo( bi );

	*status = BMMRES_SUCCESS;
	return storage;
}

BMMRES image_import::GetImageInfo( BitmapInfo *bi )
{
	int side;
	std::string ext;
	size_t off, count;

	ext = bi->Name();
	off = ext.rfind( "." ) + 1;
	count = ext.length() - off;
	ext = ext.substr( off, count );

	side = atoi( ext.c_str() );

	bi->SetWidth( side );
	bi->SetHeight( side );
	bi->SetAspect( 1.0f );
	bi->SetType( BMM_TRUE_32 );
	bi->SetFlags( MAP_HAS_ALPHA );
	bi->SetFirstFrame( 0 );
	bi->SetLastFrame( 0 );

	return BMMRES_SUCCESS;
}