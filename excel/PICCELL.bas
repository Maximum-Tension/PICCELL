Attribute VB_Name = "PICCELL"
'##############################################################################
'# BAS - PICCELL                                #       Maximum Tension       #
'# Teoman Deniz - maximum-tension.com           # License - GPL-3.0           #
'# Create - 2026/11/18                          # Update - 2026/11/21         #
'##############################################################################
'
' A PHOTO OF A TABLE (PRINTED OR HANDWRITTEN) INTO THE CELLS. RIGHT-CLICK A
' CELL AND CHOOSE "INSERT TABLE FROM PICTURE", OR PRESS CTRL+SHIFT+T.
' PICCELL.EXE (NEXT TO THIS ADD-IN) READS THE PICTURE (THE OCR OF WINDOWS 10 /
' 11 FOR PRINT, ITS OWN READER FOR HANDWRITING) AND WRITES A TAB SEPARATED
' FILE; THIS MODULE PUTS IT INTO THE CELLS AS IF IT WAS TYPED.
'
' THIS FILE STAYS ASCII: THE VBA EDITOR IMPORTS IN THE CODE PAGE OF THE PC.
' TURKISH LETTERS ARE WRITTEN {s} {g} {i} {I} {u} {o} {c} (SEE LETTERS).

Option Explicit

#If VBA7 Then
Private Declare PtrSafe Function MessageBoxW Lib "user32" ( _
    ByVal WINDOW As LongPtr, ByVal TEXT As LongPtr, _
    ByVal CAPTION As LongPtr, ByVal FLAGS As Long) As Long
#Else
Private Declare Function MessageBoxW Lib "user32" ( _
    ByVal WINDOW As Long, ByVal TEXT As Long, _
    ByVal CAPTION As Long, ByVal FLAGS As Long) As Long
#End If

Private Const MENU_TAG As String = "PICCELL" ' OUR MENU BUTTONS CARRY THIS TAG
Private Const HOT_KEYS As String = "^+T" ' CTRL+SHIFT+T
Private Const ASK_YES As Long = 6 ' IDYES

' EXIT CODES OF PICCELL.EXE (SRC/PICCELL.H)
Private Const PICCELL_DONE As Long = 0
Private Const PICCELL_NO_PICTURE As Long = 2
Private Const PICCELL_NO_CLIP As Long = 3
Private Const PICCELL_NO_OCR As Long = 4
Private Const PICCELL_NO_TEXT As Long = 5
Private Const PICCELL_NO_WRITE As Long = 6
Private Const PICCELL_NO_READER As Long = 7

' .,,,,. #==========================================================# .,,,,.
'  \\//  |                      MENU AND KEYS                       |  \\//
'   \/   #==========================================================#   \/

' EXCEL RUNS THIS WHEN IT OPENS THE ADD-IN
Public Sub Auto_Open()
    PICCELL_MENU
End Sub

' AND THIS WHEN IT CLOSES IT
Public Sub Auto_Close()
    MENU_REMOVE
End Sub

' PUTS THE BUTTONS IN THE RIGHT-CLICK MENU AND THE ADD-INS TAB. ALSO A MACRO,
' IN CASE THE MENU EVER GOES MISSING (ALT+F8, PICCELL_MENU).
Public Sub PICCELL_MENU()
    Dim BAR As Object

    MENU_REMOVE
    On Error Resume Next
    For Each BAR In Application.CommandBars
        If BAR.Name = "Cell" Or BAR.Name = "List Range Popup" Then
            ADD_BUTTON BAR, CLIP_CAPTION(), "PICCELL_FROM_CLIPBOARD", 1
            ADD_BUTTON BAR, FILE_CAPTION(), "PICCELL_FROM_FILE", 1
        End If
    Next BAR
    ' A TOOLBAR OF AN ADD-IN SHOWS UP IN THE ADD-INS TAB OF THE RIBBON
    Set BAR = Application.CommandBars.Add( _
        Name:=MENU_TAG, Position:=1, Temporary:=True)
    ADD_BUTTON BAR, FILE_CAPTION(), "PICCELL_FROM_FILE", 0
    ADD_BUTTON BAR, CLIP_CAPTION(), "PICCELL_FROM_CLIPBOARD", 0
    BAR.Visible = True
    Application.OnKey HOT_KEYS, "'" & ThisWorkbook.Name & "'!PICCELL_FROM_FILE"
    On Error GoTo 0
End Sub

Private Sub ADD_BUTTON( _
    ByVal BAR As Object, ByVal CAPTION As String, ByVal ACTION As String, _
    ByVal BEFORE As Long)
    Dim BUTTON As Object

    If BEFORE > 0 Then
        Set BUTTON = BAR.Controls.Add(Type:=1, Before:=BEFORE, Temporary:=True)
    Else
        Set BUTTON = BAR.Controls.Add(Type:=1, Temporary:=True)
    End If
    BUTTON.CAPTION = CAPTION
    BUTTON.OnAction = "'" & ThisWorkbook.Name & "'!" & ACTION
    BUTTON.Tag = MENU_TAG
    BUTTON.Style = 2 ' CAPTION ONLY
End Sub

Private Sub MENU_REMOVE()
    Dim CONTROL As Object
    Dim LIMIT As Long

    On Error Resume Next
    For LIMIT = 1 To 50
        Set CONTROL = Nothing
        Set CONTROL = Application.CommandBars.FindControl(Tag:=MENU_TAG)
        If CONTROL Is Nothing Then Exit For
        CONTROL.Delete
    Next LIMIT
    Application.CommandBars(MENU_TAG).Delete
    Application.OnKey HOT_KEYS
    On Error GoTo 0
End Sub

Private Function FILE_CAPTION() As String
    FILE_CAPTION = T("Insert table from picture...", "Resimden tablo ekle...")
End Function

Private Function CLIP_CAPTION() As String
    CLIP_CAPTION = T( _
        "Insert table from copied picture", _
        "Kopyalanan resimden tablo ekle")
End Function
'   /\   #==========================================================#   /\
'  //\\  |                      MENU AND KEYS                       |  //\\
' '""""' #==========================================================# '""""'

' .,,,,. #==========================================================# .,,,,.
'  \\//  |                       TABLE INSERT                       |  \\//
'   \/   #==========================================================#   \/

Public Sub PICCELL_FROM_FILE()
    RUN_PICCELL False
End Sub

Public Sub PICCELL_FROM_CLIPBOARD()
    RUN_PICCELL True
End Sub

Private Sub RUN_PICCELL(ByVal FROM_CLIPBOARD As Boolean)
    Dim PICTURE As Variant
    Dim EXE As String
    Dim RESULT As String
    Dim RUN_LINE As String
    Dim CHECKS As String
    Dim CODE As Long

    If TypeName(Selection) <> "Range" Then
        SAY T("Select a cell first.", "{O}nce bir h{u}cre se{c}in."), 48
        Exit Sub
    End If
    EXE = ThisWorkbook.Path & "\PICCELL.exe"
    If Dir(EXE) = "" Then
        SAY T("PICCELL.exe was not found here:", _
            "PICCELL.exe burada bulunamad{i}:") & vbCrLf & EXE, 16
        Exit Sub
    End If
    If FROM_CLIPBOARD Then
        PICTURE = "-clip"
    Else
        PICTURE = Application.GetOpenFilename( _
            T("Pictures", "Resimler") & _
            " (*.jpg;*.jpeg;*.png;*.bmp;*.gif;*.tif;*.tiff;*.heic;*.webp)," & _
            "*.jpg;*.jpeg;*.png;*.bmp;*.gif;*.tif;*.tiff;*.heic;*.webp", _
            1, T("Choose the photo of the table", _
            "Tablonun foto{g}raf{i}n{i} se{c}in"))
        If VarType(PICTURE) = vbBoolean Then Exit Sub ' CANCELLED
    End If
    RESULT = Environ$("TEMP") & "\PICCELL_" & Format$(Now, "yyyymmddhhnnss") _
        & ".tsv"
    CHECKS = Left$(RESULT, Len(RESULT) - 4) & "_check.tsv"
    RUN_LINE = """" & EXE & """ """ & PICTURE & """ """ & RESULT & """" & _
        " -check """ & CHECKS & """"
    Application.StatusBar = T("Reading the picture...", "Resim okunuyor...")
    Application.Cursor = 2 ' WAIT
    CODE = CreateObject("WScript.Shell").Run(RUN_LINE, 0, True)
    Application.Cursor = -4143 ' DEFAULT
    Application.StatusBar = False
    If CODE <> PICCELL_DONE Then
        SAY PROBLEM(CODE), 48
        Exit Sub
    End If
    FILL_CELLS RESULT, CHECKS, ActiveCell
    On Error Resume Next
    Kill RESULT
    Kill CHECKS
    On Error GoTo 0
End Sub

' THE TAB SEPARATED FILE INTO THE CELLS, STARTING AT TARGET
Private Sub FILL_CELLS( _
    ByVal RESULT As String, ByVal CHECKS As String, ByVal TARGET As Range)
    Dim STREAM As Object
    Dim TEXT As String
    Dim LINES As Variant
    Dim PARTS As Variant
    Dim AREA As Range
    Dim ROW_COUNT As Long
    Dim COL_COUNT As Long
    Dim R As Long
    Dim C As Long

    Set STREAM = CreateObject("ADODB.Stream")
    STREAM.Type = 2 ' TEXT
    STREAM.Charset = "utf-8"
    STREAM.Open
    STREAM.LoadFromFile RESULT
    TEXT = STREAM.ReadText
    STREAM.Close
    LINES = Split(TEXT, vbCrLf)
    ROW_COUNT = UBound(LINES) + 1
    Do While ROW_COUNT > 0
        If LINES(ROW_COUNT - 1) <> "" Then Exit Do
        ROW_COUNT = ROW_COUNT - 1
    Loop
    If ROW_COUNT = 0 Then
        SAY PROBLEM(PICCELL_NO_TEXT), 48
        Exit Sub
    End If
    For R = 0 To ROW_COUNT - 1
        PARTS = Split(LINES(R), vbTab)
        If UBound(PARTS) + 1 > COL_COUNT Then COL_COUNT = UBound(PARTS) + 1
    Next R
    If TARGET.Row + ROW_COUNT - 1 > TARGET.Worksheet.Rows.Count Or _
        TARGET.Column + COL_COUNT - 1 > TARGET.Worksheet.Columns.Count Then
        SAY T("The table does not fit here.", _
            "Tablo buraya s{i}{g}m{i}yor."), 48
        Exit Sub
    End If
    Set AREA = TARGET.Resize(ROW_COUNT, COL_COUNT)
    If Application.WorksheetFunction.CountA(AREA) > 0 Then
        If SAY(T( _
            "The cells are not empty. Write over them? (Undo cannot " & _
            "bring them back.)", _
            "H{u}creler bo{s} de{g}il. {U}zerine yaz{i}ls{i}n m{i}? " & _
            "(Geri al ile geri gelmez.)"), 4 + 32) <> ASK_YES Then Exit Sub
    End If
    On Error GoTo FAILED
    Application.ScreenUpdating = False
    For R = 0 To ROW_COUNT - 1
        PARTS = Split(LINES(R), vbTab)
        For C = 0 To UBound(PARTS)
            PUT_VALUE AREA.Cells(R + 1, C + 1), PARTS(C)
        Next C
    Next R
    MARK_CELLS CHECKS, AREA
    AREA.Columns.AutoFit
    Application.ScreenUpdating = True
    AREA.Select
    Exit Sub
FAILED:
    Application.ScreenUpdating = True
    SAY T("The cells could not be written (is the sheet protected?)", _
        "H{u}crelere yaz{i}lamad{i} (sayfa korumal{i} m{i}?)") & _
        vbCrLf & Err.Description, 48
End Sub

' THE CELLS PICCELL WAS NOT SURE OF (OR CORRECTED ON ITS OWN) TURN YELLOW
Private Sub MARK_CELLS(ByVal CHECKS As String, ByVal AREA As Range)
    Dim HANDLE As Integer
    Dim ROW_TEXT As String
    Dim PARTS As Variant
    Dim R As Long
    Dim C As Long

    If Dir(CHECKS) = "" Then Exit Sub
    HANDLE = FreeFile
    Open CHECKS For Input As #HANDLE
    R = 0
    Do While Not EOF(HANDLE) And R < AREA.Rows.Count
        Line Input #HANDLE, ROW_TEXT
        PARTS = Split(ROW_TEXT, vbTab)
        For C = 0 To UBound(PARTS)
            If C < AREA.Columns.Count And PARTS(C) = "1" Then
                AREA.Cells(R + 1, C + 1).Interior.Color = RGB(255, 241, 199)
            End If
        Next C
        R = R + 1
    Loop
    Close #HANDLE
End Sub

' ONE VALUE, AS IF TYPED: "1.234,56" AND "12.05.2026" BECOME A NUMBER AND A
' DATE IN THE NUMBER FORMAT OF THIS PC. NEVER A FORMULA.
Private Sub PUT_VALUE(ByVal CELL As Range, ByVal VALUE As String)
    Dim FIRST As String

    If VALUE = "" Then
        CELL.ClearContents
        Exit Sub
    End If
    FIRST = Left$(VALUE, 1)
    If FIRST = "=" Or FIRST = "+" Or FIRST = "@" Or _
        (FIRST = "-" And Not IsNumeric(Mid$(VALUE, 2))) Then
        CELL.VALUE = "'" & VALUE
    Else
        CELL.FormulaLocal = VALUE
    End If
End Sub

Private Function PROBLEM(ByVal CODE As Long) As String
    Select Case CODE
    Case PICCELL_NO_PICTURE
        PROBLEM = T("This file could not be read. Choose a JPG or PNG photo.", _
            "Bu dosya okunamad{i}. JPG veya PNG bir foto{g}raf se{c}in.")
    Case PICCELL_NO_CLIP
        PROBLEM = T( _
            "There is no picture on the clipboard. Copy the picture first.", _
            "Panoda resim yok. {O}nce resmi kopyalay{i}n.")
    Case PICCELL_NO_OCR
        PROBLEM = T( _
            "Windows text recognition is not available on this PC " & _
            "(Windows 10 or 11 is needed).", _
            "Bu bilgisayarda Windows yaz{i} tan{i}ma yok " & _
            "(Windows 10 veya 11 gerekir).")
    Case PICCELL_NO_TEXT
        PROBLEM = T( _
            "No text was found in the picture. Take the photo closer, " & _
            "straight from above and in good light.", _
            "Resimde yaz{i} bulunamad{i}. Foto{g}raf{i} daha yak{i}ndan, " & _
            "tam yukar{i}dan ve iyi {i}{s}{i}kta {c}ekin.")
    Case PICCELL_NO_WRITE
        PROBLEM = T("The result could not be written to the TEMP folder.", _
            "Sonu{c} TEMP klas{o}r{u}ne yaz{i}lamad{i}.")
    Case PICCELL_NO_READER
        PROBLEM = T( _
            "This PICCELL.exe has no handwriting reader. Use the " & _
            "PICCELL.exe of the release.", _
            "Bu PICCELL.exe'de el yaz{i}s{i} okuyucusu yok. S{u}r{u}m{u}n " & _
            "PICCELL.exe'sini kullan{i}n.")
    Case Else
        PROBLEM = T("PICCELL.exe did not work (code ", _
            "PICCELL.exe {c}al{i}{s}mad{i} (kod ") & CODE & ")."
    End Select
End Function
'   /\   #==========================================================#   /\
'  //\\  |                       TABLE INSERT                       |  //\\
' '""""' #==========================================================# '""""'

' .,,,,. #==========================================================# .,,,,.
'  \\//  |                         MESSAGES                         |  \\//
'   \/   #==========================================================#   \/

' ENGLISH OR TURKISH, BY THE LANGUAGE OF EXCEL
Private Function T(ByVal ENGLISH As String, ByVal TURKISH As String) As String
    Dim LANGUAGE As Long

    On Error Resume Next
    LANGUAGE = Application.LanguageSettings.LanguageID(2) ' UI LANGUAGE
    On Error GoTo 0
    If LANGUAGE = 1055 Then T = LETTERS(TURKISH) Else T = ENGLISH
End Function

' {s} {g} {i} {u} {o} {c} AND {S} {G} {I} {U} {O} {C} TO TURKISH LETTERS
Private Function LETTERS(ByVal TEXT As String) As String
    TEXT = Replace(TEXT, "{s}", ChrW(351), Compare:=vbBinaryCompare)
    TEXT = Replace(TEXT, "{S}", ChrW(350), Compare:=vbBinaryCompare)
    TEXT = Replace(TEXT, "{g}", ChrW(287), Compare:=vbBinaryCompare)
    TEXT = Replace(TEXT, "{G}", ChrW(286), Compare:=vbBinaryCompare)
    TEXT = Replace(TEXT, "{i}", ChrW(305), Compare:=vbBinaryCompare)
    TEXT = Replace(TEXT, "{I}", ChrW(304), Compare:=vbBinaryCompare)
    TEXT = Replace(TEXT, "{u}", ChrW(252), Compare:=vbBinaryCompare)
    TEXT = Replace(TEXT, "{U}", ChrW(220), Compare:=vbBinaryCompare)
    TEXT = Replace(TEXT, "{o}", ChrW(246), Compare:=vbBinaryCompare)
    TEXT = Replace(TEXT, "{O}", ChrW(214), Compare:=vbBinaryCompare)
    TEXT = Replace(TEXT, "{c}", ChrW(231), Compare:=vbBinaryCompare)
    TEXT = Replace(TEXT, "{C}", ChrW(199), Compare:=vbBinaryCompare)
    LETTERS = TEXT
End Function

' A MESSAGE BOX THAT SHOWS TURKISH LETTERS ON ANY PC (MSGBOX CAN NOT)
Private Function SAY(ByVal TEXT As String, ByVal FLAGS As Long) As Long
    Dim CAPTION As String

    CAPTION = "PICCELL"
    SAY = MessageBoxW(Application.Hwnd, StrPtr(TEXT), StrPtr(CAPTION), FLAGS)
End Function
'   /\   #==========================================================#   /\
'  //\\  |                         MESSAGES                         |  //\\
' '""""' #==========================================================# '""""'
