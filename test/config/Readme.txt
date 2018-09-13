
################################ Command File #################################
The encoder supports input commands simulating dynamic events like:
	- Scene Change notification
	- Key frame insertion
	- Changing the Gop Length
	- Changing the target Bit-Rate
	- Using Long Term reference Picture
	- Changing the Number of BFrames
	- Changing the Frame-rate

This feature is enabled when a command file is provided.(CmdFile in [INPUT]
section)

Each line of the file defines one frame identifier followed by one or more
commands applying to this frame.

Syntax :
		<Frame number>: <command> [, <command>]

	Where <command> can be one of the following:
		SC		----> Scene Change
		KF		----> KeyFrame (restarting new GOP)
		LT		----> Use Frmae as long term reference picture
		UseLT		----> Use long term reference picture to encode
		GopLen=<length>	----> Change the Gop length
		NumB=<numB>	----> Change the number of consecutive B-Frame
		BR=<Kbits/s>	----> Change the target bit Rate
		Fps=<frames/s>	----> Change the Frame rate




################################## ROI File ###################################
The encoder supports setting Region Of Interest and the frame at which the
ROI(s) should apply.

This feature is enabled by using QpCtrlMode = ROI_QP in [SETTINGS] section and
a ROI file is provided to specify region quality.(RoiFile in [INPUT] section)

The ROI definition starts with a line specifying the frame identifier, the
background quality, and an ROI order (in case of overlapping regions), followed
by one or more lines each defining a ROI for this frame and the following
frames until the next ROI definition.

The encoder supports setting one or several Region Of Interest(s).

Syntax :
		frame <index> : [BkgQuality=<quality>, Order=<order>]
		<posX>:<posY>, <width>x<height>, <quality>
		<posX>:<posY>, <width>x<height>, <quality>
		...

	Where:
		<posX>, <posY>, <width> and <height> are in pixel unit. They are then
			automatically rounded to the bounding CTB units.
		<quality> can be,
			HIGH_QUALITY	----> Higher quality than rate-control given value
			MEDIUM_QUALITY	----> Same quality as rate-control given value
			LOW_QUALITY	----> Lower quality than rate-control given value
			NO_QUALITY	----> Worse possible quality for region
			STATIC_QUALITY	----> Region with static content (i.e; exactly same
						content as in the reference picture)
		<order> can be,
			QUALITY_ORDER	----> Use quality of region having the best quality
			INCOMING_ORDER	----> Use quality of the latest defined region

Note: To specify only one ROI for the full sequence, use "frame ALL:"




################################### QP File ###################################
QPs of each CTB can be provided to encoder from an external buffer loaded from
a file. The file shall be named "QPs.hex" and located in the working directory.
The QP table modes are enabled by using "QPCtrlMode = LOAD_QP" or
"QPCtrlMode = LOAD_QP | RELATIVE_QP."

For AVC, there is one byte per 16x16 MB (in raster scan format):
		absolute QP in [0;51] or relative QP in [-32;31].
For HEVC, there is one byte per 32x32 CTB (in raster scan format):
		absolute QP in [0;51] or relative QP in [-32;31].

Note :	only the 6 LSBs of each byte are used for QP, the 2 MSBs
	are reserved.

For example, to specify the following relative QP table,
	-------------------------------------------------------------------------------------------------
	|	-1	|	-2	|	0	|	1	|	1	|	4	|
	-------------------------------------------------------------------------------------------------
	|	-4	|	-1	|	1	|	2	|	2	|	1	|
	-------------------------------------------------------------------------------------------------
	|	-1	|	0	|	-1	|	1	|	1	|	4	|
	-------------------------------------------------------------------------------------------------
	|	0	|	0	|	-2	|	2	|	2	|	6	|
	-------------------------------------------------------------------------------------------------
	|	1	|	-2	|	0	|	1	|	4	|	2	|
	-------------------------------------------------------------------------------------------------
The QPs.hex file for HEVC should be like:
		3F
		3E
		00
		01
		01
		04
		3C
		3F
		..
		..
		..

Note:	Table size depends on resolution. Note the QPs.hex sample file corresponds
	to a given resolution.
