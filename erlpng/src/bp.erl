-module(bp).
-compile(export_all).

start(Bmp) when is_list(Bmp) ->
    try file:read_file(Bmp) of
        {ok,Bo} -> fileHeader(Bo);
        _Error ->
            {error,"file not opened or not found.",_Error}
    catch
        _:Why -> io:format("[~p]:~p open error,\"~p\"!~n",[?MODULE,Bmp,Why])
    end.

%% BITMAPFILEHEADER
fileHeader(<<Bo:14/binary,InH/binary>>) ->
    case Bo of
        <<"BM",_Size:32/little,_:16,_:16,_:32/little>> ->
            bmpInfo(Bo,InH);
        _Why ->
            io:format("Thie not file of bmp format.~n")
    end.

%% BITMAPINFOHEADER
bmpInfo(_Bo,<<Info:40/binary,Raws/binary>>) ->
    case Info of
        <<_:32/little,Width:32/little,Height:32/little,_:28/binary>> ->
            %%_Bin = <<Bo/binary,Info/binary,Raws/binary>>,
            %%Filename = "qrcode.bmp",
            %%ok = file:write_file(Filename, _Bin),

            Png = simple_png_encode(Width, Height, Raws),
            PngFilename = "qrcode.png",
            ok = file:write_file(PngFilename, Png);
        _Why ->
            io:format("Thie file not concat bmp header information.~n")    
    end.

%% Very simple PNG encoder for demo purposes
simple_png_encode(Width,Height,Data) ->
	MAGIC = <<137, 80, 78, 71, 13, 10, 26, 10>>,
	IHDR = png_chunk(<<"IHDR">>, <<Width:32, Height:32, 8:8, 2:8, 0:24>>),
	SRGB = png_chunk(<<"sRGB">>,<<0>>),
	GAMA = png_chunk(<<"gAMA">>,<<16#B18F:32>>),
	PHYs = png_chunk(<<"pHYs">>,<<16#0B13:32,16#0B13:32,1:8>>),
    %% 因为bmp格式规定，每扫描行像素为4的倍数对齐，故而当出现不足时，应该对齐长度，数据不足部分补齐填充0x0,否则转换后的图片会出现
    %% 畸形、花屏等
    B31 = bnot(31),
    RowsCount = trunc((((Width * 24) + 31) band B31) / 8), %%3 * Width,
    Pngo = pixel(RowsCount,Data),

	PixelData = zlib:compress(Pngo),
	IDAT = png_chunk(<<"IDAT">>, PixelData),
	IEND = png_chunk(<<"IEND">>, <<>>),
	%%<<MAGIC/binary, IHDR/binary,SRGB/binary,GAMA/binary, PHYs/binary, IDAT/binary, IEND/binary>>.
    <<MAGIC/binary, IHDR/binary,SRGB/binary,GAMA/binary,PHYs/binary, IDAT/binary, IEND/binary>>.

png_chunk(Type, Bin) ->
	Length = byte_size(Bin),
	CRC = erlang:crc32(<<Type/binary, Bin/binary>>),
	<<Length:32, Type/binary, Bin/binary, CRC:32>>.

pixel(RowCount,Data) ->
    pixel0(RowCount,Data,<<>>).

pixel0(_,<<>>,Acc) -> Acc;
pixel0(Rows,<<Ro/binary>>,Acc) ->
    if byte_size(Ro) >= Rows ->
            <<Co:Rows/binary,Bo/binary>> = Ro,
            Coo = pixel1(Co,<<>>),
            pixel0(Rows, Bo,<<0, Coo/binary,Acc/binary>>);
        byte_size(Ro) < Rows ->   
            Coo = binary:copy(<<0>>,Rows),
            pixel0(Rows, <<>>,<<0, Coo/binary,Acc/binary>>)
    end.

pixel1(<<>>,Acc) -> Acc;
pixel1(<<Bo/binary>>,Acc) when (byte_size(Bo) > 0) and (byte_size(Bo) < 3) ->
    pixel1(<<>>,<<Acc/binary>>);
pixel1(<<R:1/binary,G:1/binary,B:1/binary,Bix/binary>>,Acc) ->
    %%pixel1(Bix,<<Acc/binary,B/bits,G/bits,R/bits>>).
    %%pixel1(Bix,<<Acc/binary,R/bits,G/bits,B/bits>>).
    %%pixel1(Bix,<<Acc/binary,B/binary,G/binary,R/binary,255:8>>).
    pixel1(Bix,<<Acc/binary,B/binary,G/binary,R/binary>>).
