APC smart protocol
==================

(index-Smart-protocol-262) (index-Protocol-Smart-263) The APC UPS
protocol was originally analyzed by Pavel Korensky with additions
from Andre H. Hendrick beginning in 1995, and we want to give
credit for good, hard work, where credit is due. After having said
that, you will see that Steven Freed built much of the orginal
apcupsd information file. [Comment inserted by Riccardo Facchetti]

The start of this chapter of the apcupsd manual in HTML format was
pulled from the {Network UPS Tools
(NUT)}{http://www.exploits.org/nut/library/apcsmart.html} site. It
has been an invaluable tool in improving apcupsd, and I consider it
the { Bible} of APC UPS programming. In the course of using it, I
have added information gleaned from apcupsd and information
graciously supplied by APC. Hopefully, the additions made herein
can benefit the original author and his {programming
project}{http://www.exploits.org/nut}, and maybe some day, the
apcupsd project and the { NUT} project can join forces.

(Description)

Description
-----------

{Description } {toc}{subsection}{Description}

Here's the information on the elusive APC smart signaling protocol
used by their higher end units (Back-UPS Pro, Smart-UPS,
Matrix-UPS, etc). What you see here has been collected from a
variety of sources. Some people analyzed the chatter between
PowerChute and their hardware. Others sent various characters to
the UPS and figured out what the results meant.

(RS\_002d232-differences)

RS-232 differences
------------------

{RS-232 differences } {Differences!RS-232 }
{toc}{subsection}{RS-232 differences}

Normal 9 pin serial connections have TxD on 3 and RxD on 2. APC's
smart serial ports put TxD on pin 1 and RxD on pin 2. This means
you go nowhere if you use a normal straight through serial cable.
In fact, you might even power down the load if you plug one of
those cables in. This is due to the odd routing of pins - DTR and
RTS from the PC usually wind up driving the on/off line. So, when
you open the port, they go high and \*poof\* your computer dies.

Originally this evil hack was used to connect the UPS to the PC
when this page was first being built. As you can see, I cheated and
neglected the ground (only 2 wires!) and it still worked. This
method can be used for playing around, but for professional systems
this is obviously not a viable option.

That hack didn't work out so well (damned cats), so it was retired
quite awhile back. The most practical solution was to go out and
BUY the DOS/Win version of PowerChute just for the black (smart)
cable. I recommend doing the same thing if you actually care about
this thing working properly. Of course, if you have one of the
newer packages that came with PowerChute, you already have the
cable you need.

(Diagram-for-cable-hackers)

Diagram for cable hackers
-------------------------

{Hackers!Diagram for cable } {Diagram for cable hackers }
{toc}{subsection}{Diagram for cable hackers}

If you are handy with cable creation tools, check out the
{940-0024C clone
diagram}{http://www.exploits.org/nut/library/940-0024C.jpg}. That's
the black "smart" cable normally provided with APC models sold
after 1996. The loopback pins on that diagram are used to keep
PowerChute happy by allowing cable detection. If you use the
{NUT}{http://www.exploits.org/nut/} apcsmart driver, those pins
don't matter.

Many thanks to Steve Draper for providing this scan.

For additional information on cables, see the section on custom
cables (see {Cables}{Cables}) in this manual.

(The-Smart-Protocol)

The Smart Protocol
------------------

{Protocol!Smart } {Smart Protocol } {toc}{subsection}{Smart
Protocol}

Despite the lack of official information from APC, this table has
been constructed. It's standard RS-232 serial communications at
2400 bps/8N1. Don't rush the UPS while transmitting or it may stop
talking to you. This isn't a problem with the normal single
character queries, but it really does matter for multi-char things
like "@000". Sprinkle a few calls to usleep() in your code and
everything will work a lot better.

The following table describes the single character { Code} or
command that you can send to the UPS, its meaning, and what sort of
response the UPS will provide. Typically, the response shown below
is followed by a newline (\\n in C) and a carriage return (\\r in
C). If you send the UPS a command that it does not recognize or
that is not available on your UPS, it will normally respond by "NA"
for not available, otherwise the response is given in the
"Typical results" column.

{lot}{table}{Single Character Commands}

    {Code} & {Meaning} & {Typical results} & { }
    {^A} & {Model string} & {SMART-UPS 700} & { }
    {^N} & {Turn on UPS (send twice, with 1.5s delay between chars)
    Only on 3rd gen SmartUPS and Black Back-UPS Pros} & {n/a} & { }
    {^Z} & {Permitted EEPROM Values} & {A large string (254 chars) that
    gives the EEPROM permitted values for your model. For details see
    below.} & { }
    {A} & {Front panel test} & {Light show + "OK" (and 2s beep)} & { }
    {B} & {Battery voltage} & {Ranges - typical "27.87"} & { }
    {C} & {Internal temperature (degrees C)} & {Ranges - typical
    "036.0"} & { }
    {D} & {Runtime calibration - runs until battery is below 25% (35%
    for Matrix) This updates the 'j' values - only works at 100%
    battery charge. Can be aborted with a second "D"} & {! when on
    battery, $ on line} & { }
    {E} & {Automatic self test intervals} & {Default = 336 (336 hours =
    14 days) (336=14 days, 168=7 days, ON=power on, OFF=never)} & { }
    {F} & {Line frequency, Hz} & {60.00 (50.0 in Europe)} & { }
    {G} & {Cause of transfer} & {R = unacceptable utility voltage rate
    of change, H = high utility voltage, L = low utility voltage, T =
    line voltage notch or spike, O = no transfers yet (since turnon), S
    = transfer due to serial port U command or activation of UPS test
    from front panel, NA = transfer reason still not available (read
    again).} & { }
    {K--K} & {Shutdown with grace period (set with 'p') - need 1.5s
    between first and second K} & {Matrix/3rd gen SmartUPS/Black
    Back-UPS Pros: "OK", all others: "\*"} & { }
    {L} & {Input line voltage} & {Ranges - typical "118.3" or "228.8"
    in Europe} & { }
    {M} & {Maximum line voltage received since last M query} & {Ranges
    - typical "118.9" or "230.1" in Europe} & { }
    {N} & {Minimum line voltage received since last N query} & {Ranges
    - typical "118.9" or "226.2" in Europe} & { }
    {O} & {Output voltage} & {Ranges - typical "118.3" or "228.8" in
    Europe} & { }
    {P} & {Power load %} & {Ranges - typical "011.4" depends on what
    you have plugged in.} & { }
    {Q} & {Status flags} & {Bitmapped, see below} & { }
    {R} & {Turn dumb Only on 3rd gen SmartUPS, SmartUPS v/s, BackUPS
    Pro} & {"BYE"} & { }
    {S} & {Soft shutdown after 'p' delay, return online when power
    returns Only works when UPS is on battery} & {OK} & { }
    {U} & {Simulate power failure} & {!! when switching to battery,
    then $ when back on line} & { }
    {V} & {Old firmware revision} & {"GWD" or "IWI" The last character
    indicates the locale (Domestic, International).} & { }
    {W} & {Self test (battery), results stored in "X"} & {"OK"} & { }
    {X} & {Results of last self test} & {"OK" - good battery, "BT" -
    failed due to insufficient capacity, "NG" - failed due to overload,
    "NO" - no results available (no test performed in last 5 minutes)}
    & { }
    {Y} & {Enter smart mode} & {"SM"} & { }
    {Z--Z} & {Shutdown immediately (no delay) - need 1.5s between
    first and second Z} & {N/A} & { }
    {a} & {Show protocol version.alert messages.valid commands
    (delimited by periods)} & {
    "3.!$%+?=#\|.^A^N^Z+-789 {}@ABCDEFGKLMNOPQRSUVWXYZ'abcefgjklmnopqrsuvzy {}^{}?"
    - Link-Level.alert-messages.commands} & { }
    {b} & {Firmware revision} & {"50.9.D" - 50 = SKU (variable length),
    9 = firmware revision, D = country code (D=USA, I=International,
    A=Asia, J=Japan, M=Canada)} & { }
    {c} & {UPS local id} & {UPS\_IDEN (you can program any 8 characters
    here)} & {

    }
    {e} & {Return threshold} & {% battery charge threshold for return
    (00=00%, 01=15%, 02=25%, 03=90%)} & { }
    {f} & {Battery level %} & {Ranges - typical "100.0" when fully
    charged as should normally be the case} & { }
    {g} & {Nominal battery voltage (not actual voltage - see B)} &
    {"012" or "024" or "048".} & { }
    {h} & {Measure-UPS: ambient humidity (%)} & {"nnn.n" - percentage}
    & { }
    {i} & {Measure-UPS: dry contacts} & {10 = contact 1, 20 = 2, 40 =
    3, 80 = 4} & { }
    {j} & {Estimated runtime at current load (minutes)} & {"0112:"
    (note, it is terminated with a colon)} & { }
    {k} & {Alarm delay} & {0(zero) = 5 second delay after fail, T = 30
    second delay, L = alarm at low battery only, N = no alarm} & { }
    {l} & {Low transfer voltage} & {Default "103" or "208" in Europe} &
    { }
    {m} & {Manufacturing date} & {Unique within groups of UPSes
    (production runs)} & { }
    {n} & {Serial number} & {Unique for each UPS} & { }
    {o} & {Nominal Output Voltage} & {The Nominal Output Voltage when
    running on batteries. Default "115" or "230" in Europe.} & { }
    {p} & {Shutdown grace delay, seconds} & {Default "020"
    (020/180/300/600)} & { }
    {q} & {Low battery warning, minutes} & {Default "02"} & { }
    {r} & {Wakeup delay (time) - seconds} & {Default "000"
    (000/060/180/300)} & { }
    {s} & {Sensitivity} & {"H" - highest, "M" - medium, "L" - lowest,
    "A" - autoadjust (Matrix only)} & { }
    {u} & {Upper transfer voltage} & {Default "132" or "253" in Europe}
    & { }
    {t} & {Measure-UPS: ambient temperature (degrees C)} & {"nn.nn"} &
    { }
    {x} & {Last battery change} & {Eight characters. Varies typically
    dd/mm/yy - 31/12/99} & { }
    {y} & {Copyright notice} & {"(C) APCC" - only works if firmware
    letter (from "V") is later than O} & { }
    {z} & {Reset the EEPROM to factory settings (but not ident or batt
    replacement date) Not on SmartUPS v/s or BackUPS Pro} & {"CLEAR"} &
    { }
    {+} & {Capability cycle} & {Cycle forward through possible values
    ("\|" from UPS afterward to confirm change). Do not use this unless
    you know how to program your UPS EEPROM or you may damage your
    UPS.} & { }
    {-} & {Capability cycle} & {Cycle backward through possible values
    ("\|" from UPS afterward to confirm change)Do not use this unless
    you know how to program your UPS EEPROM or you may damage your
    UPS.} & { }
    {@nnn} & {Shutdown (after delay 'p') with delayed wakeup of nnn
    tenths of an hour (after 'r' time)} & {Matrix/3rd gen UPS: "OK",
    others "\*"} & { }
    {0x7f (DEL key)} & {Abort shutdown - use to abort @, S, K--K} &
    {"OK"} & { }
    { {}} & {Register #1} & {See below} & { }
    {'} & {Register #2} & {See below} & { }
    {0} & {Battery constant} & {Set to A0 on SmartUPS 1000 with new
    battery} & { }
    {4} & {???} & {Prints 35 on SmartUPS 1000} & { }
    {5} & {???} & {Prints EF on SmartUPS 1000} & { }
    {6} & {???} & {Prints F9 on SmartUPS 1000} & { }
    {7} & {Dip switch positions (if applicable)} & {See below} & { }
    {8} & {Register #3} & {See below} & { }
    {9} & {Line quality} & {"FF" acceptable, "00" unacceptable} & { }
    {} & {Number of external battery packs attached} & {SmartCell
    models: "nnn" where nnn is how many external packs are connected
    Non-SmartCell units: whatever has been set with + and - by the
    user} & { }
    {Matrix UPS (and possibly Symmetra) specific commands} & { } & & {
    }
    {^} & {Run in bypass mode} & {If online, "BYP" is received as
    bypass mode starts If already in bypass, "INV" is received and UPS
    goes online "ERR" received if UPS is unable to transfer} & { }
    {} & {Number of bad battery packs} & {"nnn" - count of bad packs
    connected to the UPS} & { }
    {/} & {Load current} & {"nn.nn" - true RMS load current drawn by
    UPS} & { }
    {\\} & {Apparent load power} & {"nnn.nn" - output load as
    percentage of full rated load in VA.} & { }
    {^V} & {Output voltage selection (editable)} & {"A" - automatic
    according to input tap, "M" - 208 VAC, "I" - 240 VAC} & { }
    {^L} & {Front panel language} & {"E" - English, "F" - French, "G" -
    German, "S" - Spanish, "1" "2" "3" "4" - ?} & { }
    {w} & {Run time conservation} & {"NO" (disabled) or "02" "05" "08"
    - minutes of runtime to leave in battery (UPS shuts down "early")}
    & { }


(Dip-switch-info)

Dip switch info
---------------

{Dip switch info } {Info!Dip switch } {toc}{subsection}{Dip switch
info}

{lot}{table}{DIP Switch Info}

    {Bit} & {Switch} & {Option when bit=1 }
    {0} & {4} & {Low battery alarm changed from 2 to 5 mins.
    Autostartup disabled on SU370ci and 400 }
    {1} & {3} & {Audible alarm delayed 30 seconds }
    {2} & {2} & {Output transfer set to 115 VAC (from 120 VAC) or to
    240 VAC (from 230 VAC) }
    {3} & {1} & {UPS desensitized - input voltage range expanded }
    {4-7} & {-} & {Unused at this time }


(Status-bits)

Status bits
-----------

{Status bits } {Bits!Status } {toc}{subsection}{Status bits}

This is probably the most important register of the UPS, which
indicates the overall UPS status. Some common things you'll see:


-  08 = On line, battery OK

-  10 = On battery, battery OK

-  50 = On battery, battery low

-  SM = Status bit is still not available (retry reading)


{lot}{table}{UPS Status Bits}

    {Bit} & {Hex Bit} & {Meaning }
    {0} & {0x01} & {1 = Runtime calibration occurring Not reported by
    Smart UPS v/s and BackUPS Pro }
    {1} & {0x02} & {1 = SmartTrim Not reported by 1st and 2nd
    generation SmartUPS models }
    {2} & {0x04} & {1 = SmartBoost }
    {3} & {0x08} & {1 = On line (this is the normal condition) }
    {4} & {0x10} & {1 = On battery }
    {5} & {0x20} & {1 = Overloaded output }
    {6} & {0x40} & {1 = Battery low }
    {7} & {0x80} & {1 = Replace battery }


(Alert-messages)

Alert messages
--------------

{Alert messages } {Messages!Alert } {toc}{subsection}{Alert
messages}

These single character messages are sent by the UPS any time there
is an Alert condition. All other responses indicated above are sent
by the UPS only in response to a query or action command.

{lot}{table}{Alert Messages}

    {Character} & {Description }
    {!} & {Line Fail - sent when the UPS goes on-battery, repeated
    every 30 seconds until low battery condition reached. Sometimes
    occurs more than once in the first 30 seconds. }
    {$} & {Return from line fail - UPS back on line power, only sent if
    a ! has been sent. }
    {%} & {Low battery - Sent to indicate low battery, but not on
    SmartUPS v/s or BackUPS Pro models }
    {+} & {Return from low battery - Sent when the battery has been
    recharged to some level only if a % has been sent previously }
    {?} & {Abnormal condition - sent for conditions such as
    "shutdown due to overload" or
    "shutdown due to low battery capacity". Also occurs within 10
    minutes of turnon. }
    {=} & {Return from abnormal condition - Sent when the UPS returns
    from an abnormal condition where ? was sent, but not a turn-on. Not
    implemented on SmartUPS v/s or BackUPS Pro models. }
    {\*} & {About to turn off - Sent when the UPS is about to switch
    off the load. No commands are processed after this character is
    sent. Not implemented on SmartUPS v/s, BackUPS Pro, or 3rd
    generation SmartUPS models. }
    {#} & {Replace battery - Sent when the UPS detects that the battery
    needs to be replaced. Sent every 5 hours until a new battery test
    is run or the UPS is shut off. Not implemented on SmartUPS v/s or
    BackUPS Pro models. }
    {&} & {Check alarm register for fault (Measure-UPS) - sent to
    signal that temp or humidity out of set limits. Also sent when one
    of the contact closures changes states. Sent every 2 minutes, stops
    when the alarm conditions are reset. Only sent for alarms enabled
    with I. Cause of alarm may be determined with J. Not on SmartUPS
    v/s or BackUPS Pro. }
    {\|} & {Variable change in EEPROM - Sent whenever any EEPROM
    variable is changed. Only supported on Matrix UPS and 3rd
    generation SmartUPS models. }


(Register-1)

Register 1
----------

{Register 1 } {toc}{subsection}{Register 1}

All bits are valid on the Matrix UPS. SmartUPS models only support
bits 6 and 7. Other models do not respond.

{lot}{table}{Register 1 Layout}

    {Bit} & {Hex Bit} & {Meaning }
    {0} & {0x01} & {In wakeup mode (typically lasts 2s) }
    {1} & {0x02} & {In bypass mode due to internal fault - see register
    2 or 3 }
    {2} & {0x04} & {Going to bypass mode due to command }
    {3} & {0x08} & {In bypass mode due to command }
    {4} & {0x10} & {Returning from bypass mode }
    {5} & {0x20} & {In bypass mode due to manual bypass control }
    {6} & {0x40} & {Ready to power load on user command }
    {7} & {0x80} & {Ready to power load on user command or return of
    line power }


(Register-2)

Register 2
----------

{Register 2 } {toc}{subsection}{Register 2}

Matrix UPS models report bits 0-5. SmartUPS models only support
bits 4 and 6. SmartUPS v/s and BackUPS Pro report bits 4, 6, 7.
Unused bits are set to 0. Other models do not respond.

{lot}{table}{Register 2 Layout}

    {Bit} & {Meaning }
    {0} & {Fan failure in electronics, UPS in bypass }
    {1} & {Fan failure in isolation unit }
    {2} & {Bypass supply failure }
    {3} & {Output voltage select failure, UPS in bypass }
    {4} & {DC imbalance, UPS in bypass }
    {5} & {Command sent to stop bypass with no battery connected - UPS
    still in bypass }
    {6} & {Relay fault in SmartTrim or SmartBoost }
    {7} & {Bad output voltage }


(Register-3)

Register 3
----------

{Register 3 } {toc}{subsection}{Register 3}

All bits are valid on the Matrix UPS and 3rd generation SmartUPS
models. SmartUPS v/s and BackUPS Pro models report bits 0-5. All
others report 0-4. State change of bits 1,2,5,6,7 are reported
asynchronously with ? and = messages.

{lot}{table}{Register 3 Layout}

    {Bit} & {Meaning }
    {0} & {Output unpowered due to shutdown by low battery }
    {1} & {Unable to transfer to battery due to overload }
    {2} & {Main relay malfunction - UPS turned off }
    {3} & {In sleep mode from @ (maybe others) }
    {4} & {In shutdown mode from S }
    {5} & {Battery charger failure }
    {6} & {Bypass relay malfunction }
    {7} & {Normal operating temperature exceeded }


(Interpretation-of-the-Old-Firmware-Revision)

Interpretation of the Old Firmware Revision
-------------------------------------------

{Revision!Interpretation of the Old Firmware } {Interpretation of
the Old Firmware Revision } {toc}{subsection}{Interpretation of the
Old Firmware Revision}

The Old Firmware Revision is obtained with the "V" command, which
gives a typical response such as "GWD" or "IWI", and can be
interpreted as follows:

::

         Old Firmware revision and model ID String for SmartUPS \& MatrixUPS
         
         This is a three character string XYZ
         
            where X == Smart-UPS or Matrix-UPS ID Code.
              range 0-9 and A-P
                1 == unknown
                0 == Matrix 3000
                5 == Matrix 5000
              the rest are Smart-UPS and Smart-UPS-XL
                2 == 250       3 == 400       4 == 400
                6 == 600       7 == 900       8 == 1250
                9 == 2000      A == 1400      B == 1000
                C == 650       D == 420       E == 280
                F == 450       G == 700       H == 700XL
                I == 1000      J == 1000XL    K == 1400
                L == 1400XL    M == 2200      N == 2200XL
                O == 3000      P == 5000
         
            where Y == Possible Level of Smart Features, unknown???
                G == Stand Alone
                T == Stand Alone
                        V == ???
                W == Rack Mount
         
            where Z == National Model Use Only Codes
                D == Domestic        115 Volts
                I == International   230 Volts
                A == Asia ??         100 Volts
                J == Japan ??        100 Volts

(Interpretation-of-the-New-Firmware-Revision)

Interpretation of the New Firmware Revision
-------------------------------------------

{Revision!Interpretation of the New Firmware } {Interpretation of
the New Firmware Revision } {toc}{subsection}{Interpretation of the
New Firmware Revision}

::

         New Firmware revison and model ID String in NN.M.L is the format
         
             where NN == UPS ID Code.
                 12 == Back-UPS Pro 650
                 13 == Back-UPS Pro 1000
                 52 == Smart-UPS 700
                 60 == SmartUPS 1000
                 72 == Smart-UPS 1400
         
                 where NN now Nn has possible meanings.
                     N  == Class of UPS
                     1n == Back-UPS Pro
                     5n == Smart-UPS
                     7n == Smart-UPS NET
         
                      n == Level of intelligence
                     N1 == Simple Signal, if detectable WAG(*)
                     N2 == Full Set of Smart Signals
                     N3 == Micro Subset of Smart Signals
         
             where M == Possible Level of Smart Features, unknown???
                 1 == Stand Alone
                 8 == Rack Mount
                 9 == Rack Mount
         
             where L == National Model Use Only Codes
                 D == Domestic        115 Volts
                 I == International   230 Volts
                 A == Asia ??         100 Volts
                 J == Japan ??        100 Volts
                 M == North America   208 Volts (Servers)

(EEPROM-Values)

EEPROM Values
-------------

{EEPROM Values } {Values!EEPROM } {toc}{subsection}{EEPROM Values}

Upon sending a ^Z, your UPS will probably spit back approximately
254 characters something like the following (truncated here for the
example):

#uD43132135138129uM43229234239224uA43110112114108 ....

It looks bizarre and ugly, but is easily parsed. The # is some kind
of marker/ident character. Skip it. The rest fits this form:


-  Command character - use this to select the value

-  Locale - use 'b' to find out what yours is (the last character),
   '4' applies to all

-  Number of choices - '4' means there are 4 possibilities coming
   up

-  Choice length - '3' means they are all 3 chars long


Matrix-UPS models have ## between each grouping for some reason.

Here is an example broken out to be more readable:

::

          CMD DFO RSP FSZ FVL
          u   D   4   3   127 130 133 136
          u   M   4   3   229 234 239 224
          u   A   4   3   108 110 112 114
          u   I   4   3   253 257 261 265
          l   D   4   3   106 103 100 097
          l   M   4   3   177 172 168 182
          l   A   4   3   092 090 088 086
          l   I   4   3   208 204 200 196
          e   4   4   2   00   15  50  90
          o   D   1   3   115
          o   J   1   3   100
          o   I   1   3   230 240 220 225
          o   M   1   3   208
          s   4   4   1     H   M   L   L
          q   4   4   2    02  05  07  10
          p   4   4   3   020 180 300 600
          k   4   4   1     0   T   L   N
          r   4   4   3   000 060 180 300
          E   4   4   3   336 168  ON OFF
         
          CMD == UPSlink Command.
                u = upper transfer voltage
                l = lower transfer voltage
                e = return threshold
                o = output voltage
                s = sensitivity
                p = shutdown grace delay
                q = low battery warning
                k = alarm delay
                r = wakeup delay
                E = self test interval
         
          DFO == (4)-all-countries (D)omestic (I)nternational (A)sia (J)apan
                 (M) North America - servers.
          RSP == Total number possible answers returned by a given CMD.
          FSZ == Max. number of field positions to be filled.
          FVL == Values that are returned and legal.
         

(Programming-the-UPS-EEPROM)

Programming the UPS EEPROM
--------------------------

{Programming the UPS EEPROM } {EEPROM!Programming the UPS }
{toc}{subsection}{Programming the UPS EEPROM}

There are at this time a maximum of 12 different values that can be
programmed into the UPS EEPROM. They are:

{lot}{table}{Programming the UPS EEPROM}

    {Item} & {Command} & {Meaning }
    {1.} & {c} & {The UPS Id or name }
    {2.} & {x} & {The last date the batteries were replaced }
    {3.} & {u} & {The Upper Transfer Voltage }
    {4.} & {l} & {The Lower Transfer Voltage }
    {5.} & {e} & {The Return Battery Charge Percentage }
    {6.} & {o} & {The Output Voltage when on Batteries }
    {7.} & {s} & {The Sensitivity to Line Quality }
    {8.} & {p} & {The Shutdown Grace Delay }
    {9.} & {q} & {The Low Battery Warning Delay }
    {10.} & {k} & {The Alarm Delay }
    {11.} & {r} & {The Wakeup Delay }
    {12.} & {E} & {The Automatic Self Test Interval }


The first two cases (Ident and Batt date) are somewhat special in
that you tell the UPS you want to change the value, then you supply
8 characters that are saved in the EEPROM. The last ten item are
programmed by telling the UPS that you want it to cycle to the next
permitted value.

In each case, you indicate to the UPS that you want to change the
EEPROM by first sending the appropriate query command (e.g. "c" for
the UPS ID or "u" for the Upper Transfer voltage. This command is
then immediately followed by the cycle EEPROM command or "-". In
the case of the UPS Id or the battery date, you follow the cycle
command by the eight characters that you want to put in the EEPROM.
In the case of the other ten items, there is nothing more to
enter.

The UPS will respond by "OK" and approximately 5 seconds later by a
vertical bar (\|) to indicate that the EEPROM was changed.

(Acknowledgements)

Acknowledgements
----------------

{Acknowledgements } {toc}{subsection}{Acknowledgements}

The apcupsd has a rather long and tormented history. Many thanks to
the guys that, with time, contributed to the general public
knowledge.

Pavel Korensky pavelk at dator3.anet.cz, Andre M. Hedrick hedrick
at suse.de, Christopher J. Reimer reimer at doe.carleton.ca, Kevin
D. Smolkowski kevins at trigger.oslc.org, Werner Panocha wpanocha
at t-online.de, Steven Freed, {Russell
Kroll}{http://www.exploits.org/ rkroll/contact.html}.

additions by: {Kern Sibbald apcupsd-users at
lists.sourceforge.net}{http://www.apcupsd.com}
