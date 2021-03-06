<Cabbage>
form caption("Gentable Example") size(400, 300), colour(220, 220, 220), pluginid("GenT")
label bounds(8, 6, 368, 20), text("Basic Usage"), fontcolour("black")
gentable bounds(8, 30, 380, 70), tablenumber(1), tablegridcolour(0, 0, 0, 255), fill(0)
groupbox bounds(8, 110, 380, 177), text("Randomly Updated Identifiers")
gentable bounds(70, 140, 141, 119), tablenumber(1) identchannel("widgetIdent"),  
</Cabbage>
<CsoundSynthesizer>
<CsOptions>
-n -d -+rtmidi=NULL -M0 -m0d 
</CsOptions>
<CsInstruments>
; Initialize the global variables. 
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

giWave ftgen 1, 0, 4096, 10, 1, 1, 1, 1 
seed 0 
;basic usage
instr 1
    aTone oscili chnget:k("gain"), 300
    outs aTone, aTone    
endin

;WIDGET_ADVANCED_USAGE

instr 2
    if metro(1) == 1 then
        event "i", "ChangeAttributes", 0, 1
    endif
endin

instr ChangeAttributes
    SIdentifier init ""
	SIdent sprintf "pos(%d, 140) ", 100 + rnd(100)
	SIdentifier strcat SIdentifier, SIdent
	SIdent sprintf "size(%d, %d) ", abs(rnd(200))+40, abs(rnd(100))+50
	SIdentifier strcat SIdentifier, SIdent
	SIdent sprintf "tablecolour:0(%d, %d, %d) ", rnd(255), rnd(255), rnd(255)
	SIdentifier strcat SIdentifier, SIdent  
	SIdent sprintf "tablebackgroundcolour:0(%d, %d, %d) ", rnd(255), rnd(255), rnd(255)
	SIdentifier strcat SIdentifier, SIdent
	SIdent sprintf "tablegridcolour(%d, %d, %d) ", rnd(255), rnd(255), rnd(255)
	SIdentifier strcat SIdentifier, SIdent  
	SIdent sprintf "outlinethickness(%f) ", rnd(100)/50
	SIdentifier strcat SIdentifier, SIdent
	SIdent sprintf "alpha(%f) ", 50 + rnd(50)/50
	SIdentifier strcat SIdentifier, SIdent
	SIdent sprintf "visible(%d) ", (rnd(100) > 80 ? 0 : 1)
	SIdentifier strcat SIdentifier, SIdent
    ;send identifier string to Cabbage
    chnset SIdentifier, "widgetIdent"           
endin
                

</CsInstruments>
<CsScore>
;causes Csound to run for about 7000 years...
f0 z
;starts instrument 1 and runs it for a week
i1 0 z
i2 0 z
</CsScore>
</CsoundSynthesizer>
