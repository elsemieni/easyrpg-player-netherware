#NoTrayIcon
#Region ;**** Directives created by AutoIt3Wrapper_GUI ****
#AutoIt3Wrapper_Icon=..\resources\player.ico
#EndRegion ;**** Directives created by AutoIt3Wrapper_GUI ****


#include <MsgBoxConstants.au3>
#include <FileConstants.au3>

    Local $hSearch = FileFindFirstFile("*.opus")

    If $hSearch = -1 Then
        MsgBox($MB_SYSTEMMODAL, "", "Error: No opus files. Usage: Put the executable in a folder with Opus files to convert it")
		Exit
    EndIf

    Local $sFileName = "", $iResult = 0

    While 1
        $sFileName = FileFindNextFile($hSearch)
        If @error Then ExitLoop

		$fp = FileOpen ( $sFileName, $FO_BINARY )
		$fop = FileOpen ( $sFileName & ".nsfx", $FO_BINARY +  $FO_OVERWRITE )
		If $fp = -1 Then ExitLoop
		If $fop = -1 Then ExitLoop

		$buffer = FileRead ($fp)
		$content = StringToBinary(StringTrimLeft( BinaryToString ($buffer), 4 ))
		FileWrite( $fop, StringToBinary("nSFx"))
		FileWrite($fop, $content)
		FileClose($fp)
		FileClose($fop)

    WEnd

    ; Close the search handle.
    FileClose($hSearch)
	 MsgBox($MB_SYSTEMMODAL, "", "Done!")
