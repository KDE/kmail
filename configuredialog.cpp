/*
 *   kmail: KDE mail client
 *   This file: Copyright (C) 2000 Espen Sand, espen@kde.org
 *   Contains code segments and ideas from earlier kmail dialog code.
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

// Add header files alphabetically

#include <signal.h>

#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qheader.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qmultilineedit.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qregexp.h>
#include <qspinbox.h>
#include <qtabwidget.h> 
#include <qvalidator.h> 
#include <qvbox.h>

#include <kapp.h>
#include <kcolorbtn.h>
#include <kconfig.h>
#include <kfiledialog.h>
#include <kfontdialog.h>
#include <kiconloader.h>
#include <klistview.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpgp.h>
#include <ksimpleconfig.h>
#include <kstddirs.h>
#include <kglobalsettings.h>


#include "accountdialog.h"
#include "colorlistbox.h"
#include "configuredialog.h"
#include "kfontutils.h"
#include "kmaccount.h"
#include "kmacctmgr.h"
#include "kmacctseldlg.h"
#include "kmfolder.h"
#include "kmglobal.h"
#include "kmidentity.h"
#include "kmmessage.h"
#include "kmsender.h"
#include "kmtopwidget.h"

#include "configuredialog.moc"


//
// I include some of the images for now to reduce file adm.
//


const char * network_xpm[] = {
"32 32 12 1",
"A	c #A699A289A699",
"       c None",
".	c #000000000000",
"X	c #C71BC30BC71B",
"o	c #861782078617",
"O	c #FFFFFFFFFFFF",
"+	c #596559655965",
"@	c #861700000000",
"#	c #000082078617",
"$	c #000000008617",
"%	c #C71B00000000",
"&	c #0000FFFF0000",
"                 ...............",
"                .AAAAAAAAAAAAAX.",
"               .AAAAAAAAAAAAAXo.",
"              .OOOOOOOOOOOOOXoo.",
"             .XXXXXXXXXXXXXXooo.",
"             .X++++++++++++Xooo.",
"             .X+@@@@@@@@@@OXooo.",
"             .X+@@@@@@@@@@OXooo.",
"             .X+@@@@@@@@@@OXooo.",
"             .X+@@@@@@@@@@OXooo.",
"             .X+@@@@@@@@@@OXooo.",
"    ...............@@@@@@@OXooo.",
"   .AAAAAAAAAAAAAX.OOOOOOOOXoo. ",
"  .AAAAAAAAAAAAAXo.XXXXXXXXXo+. ",
" .OOOOOOOOOOOOOXoo.++++++++++oX.",
".XXXXXXXXXXXXXXooo.ooooooooooXo.",
".X++++++++++++Xooo.OOOOOOOOOXoo.",
".X+##########OXooo.XXXXXXXXXooo.",
".X+##########OXooo.XXX     Xoo. ",
".X+##########OXooo.XXXXXXXXXo.  ",
".X+##########OXooo............  ",
".X+##########OXooo.        $    ",
".X+##########OXooo.       $$    ",
".XOOOOOOOOOOOOXoo.        $     ",
".XXXXXXXXXXXXXXo+.       $$     ",
" .++++++++++++++oX.      $      ",
" .ooooooooooooooXo.     $$      ",
".OOOOOOOOOOOOOOXoo.     $       ",
".XXXXXXXXXXXXXXooo.$$$$$$       ",
".X%%&&XXX     Xoo.              ",
".XXXXXXXXXXXXXXo.               ",
" ...............                "
};








const char * security_xpm[] = {
"32 32 17 1",
" 	c None",
".	c #000000000000",
"X	c #79E779E779E7",
"o	c #FFFFFFFFFFFF",
"O	c #79E779E70000",
"+	c #BEFBBEFBBEFB",
"@	c #FFFFFFFF0000",
"#	c #FFFFF7DE69A6",
"$	c #FFFFF7DE71C6",
"%	c #FFFFF7DE6185",
"&	c #FFFFFFFF4924",
"*	c #FFFFFFFF5144",
"=	c #FFFFFFFF38E3",
"-	c #FFFFFFFF4103",
";	c #FFFFF7DE79E7",
":	c #FFFFF7DE8617",
">	c #FFFFF7DE8E38",
"                   .....        ",
"                  .XoooX.       ",
"               .XXXXoXXX.XXX    ",
"             ..O++++.X+X.oooX   ",
"            .OO@@...XX+...++oX  ",
"           .O+@@.oX..#X.XX.++oX ",
"          .O+@@@.+o..#$X+X.o++oX",
"          .O+@.@.++o.#$XoX.+++oX",
"         .O+@.@.@.XX%#$X..+o++oX",
"         .O@+@.@.&*%%#$XX+o+++oX",
"         .O+@+@.@.*%%#$XoX+o++oX",
"         .O@+@+@.@.%%#$.oXo++oX ",
"         .O+@+=+&+%%%#.+oX++oX  ",
"         .O@+=-&&*%%%X.+oX+oX   ",
"        .O.X=--**%%XX.++oXoX    ",
"       .O.X=-XXXXXX..++oXoX     ",
"      .O.X--&......o++oX+oX     ",
"     .O.X&&&. .X++o++oXX++oX    ",
"    .O.X***X   .Xo++oX+X++oX    ",
"   .O.X%%%%X   .XX+oX.+X+X.     ",
"  .O.X%%OOX    .+X+oX.+X+.      ",
" .O.X##..X     .+X++oX+X+oX     ",
".O.X$$$X       .+X++oX+X++oX    ",
"..X;;OOX       .+X+X..+X+X.     ",
".X::..X        .+X+.o.+X+.      ",
".OO>X          .+X+oX.+X+oX     ",
" ..X           .+X++oX+X+oX     ",
"               .+X+X..+++XX     ",
"               .+X+.X..++.      ",
"               .+X+o.  ..       ",
"                .++.            ",
"                 ..             "
};


const char *user_xpm[] = {
"    32    32      159            2",
".. c none",
".# c #020204",
".a c #060204",
".b c #060604",
".c c #0a0a04",
".d c #16160c",
".e c #161614",
".f c #26261c",
".g c #2a2a14",
".h c #363614",
".i c #3a3a24",
".j c #3e3e14",
".k c #3e3e24",
".l c #464214",
".m c #46421c",
".n c #464224",
".o c #464624",
".p c #464634",
".q c #4a4a1c",
".r c #4e4e3c",
".s c #525224",
".t c #565624",
".u c #5a5624",
".v c #5e5a2c",
".w c #5e5e24",
".x c #625e24",
".y c #626224",
".z c #66662c",
".A c #6e6a34",
".B c #6e6e34",
".C c #727234",
".D c #7e7a24",
".E c #827e3c",
".F c #8a862c",
".G c #8a863c",
".H c #8e8a3c",
".I c #8e8a5c",
".J c #928e34",
".K c #969234",
".L c #9a922c",
".M c #9a963c",
".N c #9e9e8c",
".O c #a6a23c",
".P c #aaa234",
".Q c #aaa634",
".R c #aaa63c",
".S c #aaa64c",
".T c #aaaa8c",
".U c #aea64c",
".V c #aeaa34",
".W c #aeaa54",
".X c #b2aa2c",
".Y c #b2aa4c",
".Z c #b2aa54",
".0 c #b2ae34",
".1 c #b2ae3c",
".2 c #b2ae4c",
".3 c #b2ae54",
".4 c #b6ae4c",
".5 c #b6ae54",
".6 c #b6b234",
".7 c #b6b24c",
".8 c #b6b254",
".9 c #bab244",
"#. c #bab24c",
"## c #bab254",
"#a c #bab63c",
"#b c #bab654",
"#c c #beb64c",
"#d c #beb654",
"#e c #beba54",
"#f c #c2ba4c",
"#g c #c2ba54",
"#h c #c2be4c",
"#i c #c2be54",
"#j c #c6be4c",
"#k c #c6be54",
"#l c #c6c254",
"#m c #cac24c",
"#n c #cac254",
"#o c #cac64c",
"#p c #cac654",
"#q c #cec64c",
"#r c #cec654",
"#s c #ceca4c",
"#t c #ceca54",
"#u c #d2ca34",
"#v c #d2ca4c",
"#w c #d2ca54",
"#x c #d2ce4c",
"#y c #d2ce54",
"#z c #d6ce4c",
"#A c #d6ce54",
"#B c #d6d24c",
"#C c #d6d254",
"#D c #dad24c",
"#E c #dad254",
"#F c #dad654",
"#G c #ded654",
"#H c #deda5c",
"#I c #e2d654",
"#J c #e2d65c",
"#K c #e2da4c",
"#L c #e2da54",
"#M c #e2da5c",
"#N c #e2de54",
"#O c #e6de54",
"#P c #e6de5c",
"#Q c #e6e254",
"#R c #e6e25c",
"#S c #e6e264",
"#T c #eae254",
"#U c #eae25c",
"#V c #eae654",
"#W c #eae65c",
"#X c #eae664",
"#Y c #eae66c",
"#Z c #eee654",
"#0 c #eee65c",
"#1 c #eee664",
"#2 c #eee66c",
"#3 c #eeea54",
"#4 c #eeea64",
"#5 c #eeea6c",
"#6 c #f2ea54",
"#7 c #f2ea5c",
"#8 c #f2ea64",
"#9 c #f2ea74",
"a. c #f2ea7c",
"a# c #f2ee5c",
"aa c #f6ee54",
"ab c #f6ee5c",
"ac c #f6ee6c",
"ad c #f6ee7c",
"ae c #faf25c",
"af c #faf264",
"ag c #faf274",
"ah c #faf284",
"ai c #faf28c",
"aj c #faf67c",
"ak c #fafaec",
"al c #fef67c",
"am c #fef694",
"an c #fefa8c",
"ao c #fefa94",
"ap c #fefa9c",
"aq c #fefaa4",
"ar c #fefaac",
"as c #fefab4",
"at c #fefeac",
"au c #fefeb4",
"av c #fefebc",
"aw c #fefec4",
"ax c #fefecc",
"ay c #fefed4",
"az c #fefedc",
"aA c #fefee4",
"aB c #fefeec",
"aC c #fefef4",
"...........................#.#.#.#.#.#.#........................",
".....................d.#.##0#3#W#W#R#U#H.#.#.e..................",
".................#.##T#6a##0#Z#Z#T#Q#O#L#G#F#C.#.#..............",
"...............##Z#7aaa##0#6#0#0#V#Q#Q#M#J#E#A#r#z.b............",
".............##Wabafagajagac#4#U#Q#Q#O#N#L#G#B#y#q#E.#..........",
"...........#abaaajaoatatapah#9#X#Q#O#N#L#G#D#E#B#t#q#j.#........",
".........##8aeajasaxazaxarai#9#X#Q#N#L#I#I#F#D#z#w#o#o#..#......",
".......##TaeajasaAaCaB.N.I#Q#5#U#N#O#O.E.G#D#A#B#x#w#n#f#b.b....",
".......##7afapazaCak.T.r.p.F#P#R#L#G.H.m.o.J#w#y#s#s#o#f#f.#....",
".....##Ta#alawaAaCaC.C.n.i.s#R#P#L#L.w.k.i.t#D#v#w#r#n#k#f#..#..",
".....##6abanawayazax.C.i.f.n#R#O#G#I.w.h.f.o#C#x#v#s#m#k#c.8.#..",
".....#ababaoauavawaw.B.f.#.h#G#N#J#G.v.f.e.j#v#s#t#r#n#f#e.4.#..",
"...##T#6#7ahaqaqamai.z.#.#.l#G#G#G#G.u.#.#.q#w#s#q#m#l#f#f.4.4.#",
"...##0#Z#7#9adada.#2#X.l.m#G#I#G#E#E#z.h.i#z#x#s#p#r#j#i#b.5.Z.#",
"...##3#3#0#1#2#Y#S#P#N#G#G#G#G#D#F#A#z#z#z#x#t#s#m#n#j#f#b.7.Y.#",
"...##T#Z#V#0#R#U#U#R#M#H#G#G#D#E#z#B#A#A#y#t#v#r#p#m#g#f#b.2.W.#",
"...##U#T#Q#W#Q#R#z#K#K#G#F#G#D#E#A#z#x#v#s#t#o#m#c#h#e#c##.Y.W.#",
"...##Q#Q#Q#O#O#K.F#u#I#G#D#D#E#z#z#y#A#t#w#o#r#f.D#a#f#b.4.Y.W.#",
"...c#O#O#N#N#N#K.v#j#G#G#B#B#C#A#v#v#x#q#r#o#o#b.u.1#c#b.4.Z.2.#",
"...##N#O#K#L#L#G.#.V#D#z#E#A#z#x#y#t#t#p#s#n#j.L.#.X#b.4.Z.Y.4.#",
".....##G#L#I#G#D.X.#.V#x#z#A#w#v#x#v#q#r#q#j.Q.#.1#d#b.7.Z.Z.#..",
".....##C#E#D#G#D#o.#.y.0#y#x#x#t#r#r#p#m#j.P.x.##c#b.4.Z.Y.U.#..",
".....##B#C#z#C#z#v.P.#.x#q#w#x#s#s#q#p#m.4.u.#.R#..8.2.W.Z.O.#..",
".......##q#A#z#A#z#v.6.#.#.0#f#h#f#e.9.Y.#.#.K#..8.4.Y.W.S.#....",
".......##s#t#s#t#x#t#q#a.E.#.#.#.#.#.#.#.J.X#..4.Z.Z.Y.S.O.#....",
".........##f#r#q#p#q#o#q#m.O.E.A.A.A.F.M#c##.4.5.2.W.W.U.b......",
"...........##t#j#n#m#m#j#j#j#h#h#g#d#c#b.7.3.2.Y.W.Y.W.#........",
".............##i#c#e#h#i#f#g#c#c#b#b##.7.4.2.Z.Y.W.3.#..........",
"...............#.7.3.4#b#b#b#.#..7.2.Z.3.Z.Y.Z.U.O.b............",
".................#.#.2.W.4.4.5.Z.Z.Z.Y.W.W.Z.W.#.a..............",
".....................#.#.#.Y.W.Y.Y.Y.Y.O.g.#.#..................",
"...........................#.#.#.#.#.a.#........................"
};



const char * composer_xpm[]={
"32 32 17 1",
"i c #808080",
"j c #c3c3c3",
"o c #000080",
"l c #c0c0c0",
"f c #808000",
"d c #ffffbe",
"# c #000000",
"c c #ffa85a",
"e c #ff8000",
"b c #400000",
"n c #004040",
"g c #303030",
"a c #800000",
". c None",
"k c #ffffc0",
"m c #585858",
"h c #ffffff",
"................................",
"................................",
"................................",
"................................",
"................................",
"............................###.",
"...........................#abb#",
"..........................#ca#a#",
".........................#cdeaa#",
"........................#cdeef#.",
".............##........#cdeef#..",
"............#..#......#cdeef#...",
".........##.#..#.....#cdeef#....",
"........#..#.g###...#cdeef#.....",
".....##.#..###hii###cdeef#......",
"....#..#.g##hiihhh#cdeef#.......",
".##.#..###hiihhjj#cdeef#........",
"#..#.g##hiihhjjhh#dgef###.......",
"#..###hiihhjjhhi#kgdf#hii##.....",
".g##hiilhjjhhiih#gk##iihhjl##...",
"##hiilhjjhhiihhj###iihhjjhhhl##.",
"##mmljjhhiihhjjhhgihhjjhhhlmm#..",
"#miimmhiihhjjhhiiggjjhhhlmmll##.",
"##mmiimmhjjhhiihhggghhlmmllmm#..",
"##nnmmiimmhiihhjjggggmmllmmnn##.",
"..##nnmmiimmljjhhggmmilmmnn##...",
"....##nnmmilmmlhlmmiimmnn##.....",
"......##nnmmllmmmiimmnn##.......",
"........##nommlllmmnn##.........",
"..........##oommmoo##...........",
"............##ooo##.............",
"..............###..............."
};


const char *appearance_xpm[] = {
"    32    32       12            1",
". c #000000",
"# c #c0c0c0",
"a c #ffffff",
"b c #808080",
"c c #a0a0a4",
"d c #000080",
"e c #008080",
"f c #0000be",
"g c #00bebe",
"h c #0000ff",
"i c #00ffff",
"j c #ffffbe",
"................................",
".iiiiiiiiiiiiiiiiiiiiiiiiiiiiii.",
".igggggggggggggggggggggggggggge.",
".iggg........................ge.",
".iggg.aaaaaaaaaaaaaaaaaaaaab.ge.",
".iggg.a.......hhhhhh.......b.ge.",
".iggg.a.a#.a#.ffffff.a#.a#.b.ge.",
".iggg.a.#b.#b.dddddd.#b.#b.b.ge.",
".iggg.a.......dddddd.......b.ge.",
".iggg.a####################b.ge.",
".iggg.a####################b.ge.",
".iggg.abbbbbbbbbbbbbbbbbbbbb.ge.",
".iggg.ab..................ab.ge.",
".iggg.ab.jjjjjjjjjjjjj.a#.ab.ge.",
".iggg.ab.jjjjjjjjjjjjj.#b.ab.ge.",
".iggg.ab.jjjjjjjjjjjjj....ab.ge.",
".iggg.ab.jjjjjjjjjjjjj.cc.ab.ge.",
".iggg.ab.jjjjjjjjjjjjj.cc.ab.ge.",
".iggg.ab.jjjjjjjjjjjjj.cc.ab.ge.",
".iggg.ab.jjjjjjjjjjjjj.cc.ab.ge.",
".iggg.ab.jjjjjjjjjjjjj....ab.ge.",
".iggg.ab.jjjjjjjjjjjjj.a#.ab.ge.",
".iggg.ab.jjjjjjjjjjjjj.#b.ab.ge.",
".iggg.ab..................ab.ge.",
".iggg.aaaaaaaaaaaaaaaaaaaaab.ge.",
".iggg.bbbbbbbbbbbbbbbbbbbbbb.ge.",
".iggg........................ge.",
".igggggggggggggggggggggggggggge.",
".igggggggggggggggggggggggggggge.",
".igggggggggggggggggggggggggggge.",
".ieeeeeeeeeeeeeeeeeeeeeeeeeeeee.",
"................................"
};



const char * mime_xpm[] = {
"32 32 201 2",
"  	c None",
". 	c #020204",
"+ 	c #D2C694",
"@ 	c #FEFAD4",
"# 	c #FEFED4",
"$ 	c #D2CE8C",
"% 	c #CEBE8C",
"& 	c #FEFEE4",
"* 	c #FEFEDC",
"= 	c #E2DE84",
"- 	c #CECE94",
"; 	c #BEA27C",
"> 	c #FEFEEC",
", 	c #FAFAAC",
"' 	c #16162C",
") 	c #32328C",
"! 	c #2A2A7C",
"~ 	c #26267C",
"{ 	c #3A3A74",
"] 	c #5E5E64",
"^ 	c #FEFECC",
"/ 	c #F6F2B4",
"( 	c #4E4ECC",
"_ 	c #4646BC",
": 	c #4242C4",
"< 	c #4242BC",
"[ 	c #3E3EB4",
"} 	c #3A3AAC",
"| 	c #32329C",
"1 	c #22226C",
"2 	c #6E6E64",
"3 	c #E2E2BC",
"4 	c #FEFEC4",
"5 	c #FEFEBC",
"6 	c #5252CC",
"7 	c #4A4ACC",
"8 	c #4A4AC4",
"9 	c #3E3EBC",
"0 	c #3636BC",
"a 	c #3636B4",
"b 	c #3A3AB4",
"c 	c #3636A4",
"d 	c #222274",
"e 	c #4A4A4C",
"f 	c #FEFEAC",
"g 	c #FAEEB4",
"h 	c #DAD25C",
"i 	c #6E6EB4",
"j 	c #FEFEFC",
"k 	c #3636AC",
"l 	c #3232A4",
"m 	c #2E2E94",
"n 	c #666654",
"o 	c #FAFABC",
"p 	c #FEFAB4",
"q 	c #FEFEA4",
"r 	c #F2EE94",
"s 	c #A69E3C",
"t 	c #12123C",
"u 	c #5656DC",
"v 	c #5252D4",
"w 	c #5E5ED4",
"x 	c #3232B4",
"y 	c #36369C",
"z 	c #2A2A84",
"A 	c #161654",
"B 	c #7E7E5C",
"C 	c #EAE694",
"D 	c #E6E68C",
"E 	c #E6E29C",
"F 	c #EAE68C",
"G 	c #EEEA94",
"H 	c #DED66C",
"I 	c #767624",
"J 	c #5252DC",
"K 	c #5656E4",
"L 	c #2E2E8C",
"M 	c #1E1E2C",
"N 	c #D2CA74",
"O 	c #D2CE74",
"P 	c #D2CE6C",
"Q 	c #DAD674",
"R 	c #C6BE54",
"S 	c #4E4EDC",
"T 	c #5A5ADC",
"U 	c #5A5AEC",
"V 	c #5A5AE4",
"W 	c #4242CC",
"X 	c #3E3EC4",
"Y 	c #262674",
"Z 	c #0E0E4C",
"` 	c #9A964C",
" .	c #CEC66C",
"..	c #C6C26C",
"+.	c #C6C264",
"@.	c #CABE64",
"#.	c #BAB23C",
"$.	c #5E5EEC",
"%.	c #3E3EAC",
"&.	c #1A1A54",
"*.	c #524E24",
"=.	c #D2CA6C",
"-.	c #2E2E84",
";.	c #26266C",
">.	c #1A1A64",
",.	c #322E1C",
"'.	c #D6CE6C",
").	c #CAC264",
"!.	c #CABE5C",
"~.	c #2E2EB4",
"{.	c #3A3AA4",
"].	c #161644",
"^.	c #D6D274",
"/.	c #BEB244",
"(.	c #4E4EC4",
"_.	c #2A2AA4",
":.	c #222264",
"<.	c #3A3624",
"[.	c #D6CA6C",
"}.	c #C6C25C",
"|.	c #BEB63C",
"1.	c #4646C4",
"2.	c #4A4ABC",
"3.	c #26269C",
"4.	c #2A2A74",
"5.	c #1E1E5C",
"6.	c #0A0A2C",
"7.	c #7E7A3C",
"8.	c #CECA6C",
"9.	c #323294",
"0.	c #06061C",
"a.	c #BAB65C",
"b.	c #9E9E7C",
"c.	c #1E1A14",
"d.	c #A69E54",
"e.	c #CAC664",
"f.	c #CAC25C",
"g.	c #BEBA5C",
"h.	c #D6C294",
"i.	c #FEFEB4",
"j.	c #CECE9C",
"k.	c #363224",
"l.	c #2E2E9C",
"m.	c #2E2EA4",
"n.	c #1A1A4C",
"o.	c #0E0E34",
"p.	c #86823C",
"q.	c #D6D26C",
"r.	c #E2DA74",
"s.	c #FAF6B4",
"t.	c #4E4E44",
"u.	c #2E2E7C",
"v.	c #1E1E24",
"w.	c #CECA64",
"x.	c #DED67C",
"y.	c #EAE284",
"z.	c #EAEA84",
"A.	c #7A7A2C",
"B.	c #86822C",
"C.	c #EAE27C",
"D.	c #E6E69C",
"E.	c #26223C",
"F.	c #2A2A94",
"G.	c #262684",
"H.	c #F2EE8C",
"I.	c #FAF694",
"J.	c #F2EA8C",
"K.	c #FEF69C",
"L.	c #F6F29C",
"M.	c #E6E28C",
"N.	c #A29E54",
"O.	c #4E4A44",
"P.	c #E6E284",
"Q.	c #FEFA94",
"R.	c #FAFA94",
"S.	c #BEBE54",
"T.	c #C2BA4C",
"U.	c #EAEA7C",
"V.	c #FEFA9C",
"W.	c #FAFAA4",
"X.	c #EAE684",
"Y.	c #E6DE7C",
"Z.	c #EAEA8C",
"`.	c #F6F294",
" +	c #FEFE9C",
".+	c #E2D27C",
"++	c #F6EE8C",
"@+	c #F6F69C",
"#+	c #FAFA9C",
"$+	c #FEFE94",
"%+	c #E6E26C",
"&+	c #BEB644",
"*+	c #E6D66C",
"=+	c #F6F284",
"-+	c #DAD664",
";+	c #F6EA84",
">+	c #FEFE8C",
",+	c #A2A234",
"'+	c #FEEE94",
")+	c #1A1A04",
"!+	c #161604",
"                        . .                                     ",
"                        . + . .                                 ",
"                        . @ # $ . . .                           ",
"                      . % & * & = - ; . . . .                   ",
"                      . @ > > > > > & # , % ; . . . .           ",
"                    . ' ) ! ! ~ { ] > * * # ^ / = - ; . . .     ",
"                  . ( _ : < [ } } | 1 2 3 # ^ 4 5 5 , = $ ; . . ",
"                . 6 7 8 9 0 a b } c | d e 3 ^ ^ 5 4 5 5 f g h . ",
"              . ( 6 ( 6 i j j i k } l m 1 n o 4 p p f , q r s . ",
"            t u v u u w j j j j x [ y | z A B C D E E F G H I . ",
"            . u J J K w i j j i a [ c | L d M N O P P P Q R .   ",
"          . S J T U V S S W X 9 9 [ } c z Y Z `  ...+.+.@.#..   ",
"          . 6 J u $.U S S W X 9 [ %.} c L ! &.*.=.@.@.+.R #..   ",
"          . 6 u u K S j j j j j x [ c | -.;.>.,.'.@.).!.R s .   ",
"          . ( 6 6 u J J J j j j ~.} {.m L 1 ].,.^.@.@.@./.I .   ",
"          . 8 (.S v v v v j j j _.%.c | ~ :.t <.[.@.).}.|..     ",
"          . 1.2.(.8 8 8 8 j j j 3.c | L 4.5.6.7.8.).@.R /..     ",
"        . . < : _ 2.8 8 7 j j j 3.c 9.! 1 ].0.a.@.@.).+.s .     ",
"      . % b.. 9 9 < 2.2.2.j j j c c z Y :.t e ).@.).).R I .     ",
"      . 5 4 . 9.< < 2.j j j j j j j 4.;.].c.d.e.}.f.=.g..       ",
"    . h.5 i.j.k.l.} k l l m.m.3.d d 5.n.o.p.).).e.q.r.s . .     ",
"  . % s.f i.^ - t.~ | l | 9.L u.~ :.&.v.p.=.w.P x.y.z.A.. . . . ",
". B.C.f f q i.i.D.B E.A F.-.G.d >.o.t.d.w.P ^.= H.I.q.. . . . . ",
"  . . /.J.K.f i.f L.M.N.O.k.k.<.7.a.P Q H P.J.I.Q.R.S.. . . .   ",
"      . . T.U.V.f W.L.J.X.X.C.Y.Y.= P.Z.`.R. + + +J.s . . .     ",
"          . . /..+++W.I.`.r G r r `.@+V.#+ + +$+$+%+I . . .     ",
"              . . . &+*+C K.p  +q  + + + +$+ +$+=+s . . .       ",
"                    . . . &+-+;+q q $+ +$+>+>+>+h I . .         ",
"                          . . . &+.+U.Q. +$+Q.=+,+. .           ",
"                                . . . /.-+z.'+-+I .             ",
"                                      )+!+. ,+I .               ",
"                                            . .                 "
};










#define DEFAULT_EDITOR_STR "kedit %f"


ConfigureDialog::ApplicationLaunch::ApplicationLaunch( const QString &cmd ) 
{ 
  mCmdline = cmd; 
}

void ConfigureDialog::ApplicationLaunch::doIt( void ) 
{
  // This isn't used anywhere else so
  // it should be safe to do this here.
  // I dont' see how we can cleanly wait
  // on all possible childs in this app so
  // I use this hack instead.  Another
  // alternative is to fork() twice, recursively,
  // but that is slower.
  signal(SIGCHLD, SIG_IGN);
  system((const char *)mCmdline );
}

void ConfigureDialog::ApplicationLaunch::run( void ) 
{
  signal(SIGCHLD, SIG_IGN);  // see comment above.
  if( fork() == 0 ) 
  {
    doIt();
    exit(0);
  }
}


ConfigureDialog::ListView::ListView( QWidget *parent, const char *name,
				     int visibleItem ) 
  : KListView( parent, name )
{
  setVisibleItem(visibleItem);
}


void ConfigureDialog::ListView::resizeEvent( QResizeEvent *e )
{
  KListView::resizeEvent(e);
  resizeColums();
}


void ConfigureDialog::ListView::showEvent( QShowEvent *e )
{
  KListView::showEvent(e);
  resizeColums();
}


void ConfigureDialog::ListView::resizeColums( void )
{
  int c = columns();
  if( c == 0 )
  {
    return;
  }

  int w1 = viewport()->width();
  int w2 = w1 / c;
  int w3 = w1 - (c-1)*w2;
  
  for( int i=0; i<c-1; i++ )
  {
    setColumnWidth( i, w2 );
  }
  setColumnWidth( c-1, w3 );
}


void ConfigureDialog::ListView::setVisibleItem( int visibleItem, 
						bool updateSize )
{
  mVisibleItem = QMAX( 1, visibleItem );
  if( updateSize == true )
  {
    QSize s = sizeHint();
    setMinimumSize( s.width() + verticalScrollBar()->sizeHint().width() + 
		    lineWidth() * 2, s.height() );
  }
}


QSize ConfigureDialog::ListView::sizeHint( void ) const
{
  QSize s = QListView::sizeHint();
  
  int h = fontMetrics().height() + 2*itemMargin();
  if( h % 2 > 0 ) { h++; }
  
  s.setHeight( h*mVisibleItem + lineWidth()*2 + header()->sizeHint().height());
  return( s );
}












NewIdentityDialog::NewIdentityDialog( QWidget *parent, const char *name, 
				      bool modal )
  :KDialogBase( parent, name, modal, i18n("New Identity"), Ok|Cancel|Help, Ok,
		true )
{
  QFrame *page = makeMainWidget();
  QGridLayout *glay = new QGridLayout( page, 6, 2, 0, spacingHint() );
  glay->addColSpacing( 1, fontMetrics().maxWidth()*15 );
  glay->setRowStretch( 5, 10 );

  QLabel *label = new QLabel( i18n("New Identity:"), page );
  glay->addWidget( label, 0, 0 );
  
  mLineEdit = new QLineEdit( page );
  glay->addWidget( mLineEdit, 0, 1 );

  QButtonGroup *buttonGroup = new QButtonGroup( page );
  connect( buttonGroup, SIGNAL(clicked(int)), this, SLOT(radioClicked(int)) );
  buttonGroup->hide();

  QRadioButton *radioEmpty = 
    new QRadioButton( i18n("With empty fields"), page );
  buttonGroup->insert(radioEmpty, Empty );
  glay->addMultiCellWidget( radioEmpty, 1, 1, 0, 1 );

  QRadioButton *radioControlCenter = 
    new QRadioButton( i18n("Use Control Center settings"), page );
  buttonGroup->insert(radioControlCenter, ControlCenter );
  glay->addMultiCellWidget( radioControlCenter, 2, 2, 0, 1 );

  QRadioButton *radioDuplicate = 
    new QRadioButton( i18n("Duplicate existing identity"), page );
  buttonGroup->insert(radioDuplicate, ExistingEntry );
  glay->addMultiCellWidget( radioDuplicate, 3, 3, 0, 1 );

  mComboLabel = new QLabel( i18n("Existing identities:"), page );
  glay->addWidget( mComboLabel, 4, 0 );

  mComboBox = new QComboBox( false, page );
  glay->addWidget( mComboBox, 4, 1 );

  buttonGroup->setButton(0);
  radioClicked(0);
}


void NewIdentityDialog::slotOk( void )
{
  QString identity = identityText().stripWhiteSpace();
  if( identity.isEmpty() == true )
  {
    KMessageBox::error( this, i18n("You must specify an identity") );
    return;
  }
  
  for( int i=0; i<mComboBox->count(); i++ )
  {
    if( identity == mComboBox->text(i) )
    {
      KMessageBox::error( this, i18n("The identity already exist") );
      return;
    }
  }
  accept();
}



void NewIdentityDialog::radioClicked( int id )
{
  mDuplicateMode = id;

  bool state = mDuplicateMode == 2;
  mComboLabel->setEnabled( state );
  mComboBox->setEnabled( state );
}


void NewIdentityDialog::setIdentities( const QStringList &list )
{
  mComboBox->clear();
  mComboBox->insertStringList( list );
}


QString NewIdentityDialog::identityText( void )
{
  return( mLineEdit->text() );
}


QString NewIdentityDialog::duplicateText( void )
{
  return( mComboBox->isEnabled() ? mComboBox->currentText() : QString::null );
}


int NewIdentityDialog::duplicateMode( void )
{
  return( mDuplicateMode );
}



RenameIdentityDialog::RenameIdentityDialog( QWidget *parent, const char *name, 
					    bool modal )
  :KDialogBase( parent, name, modal, i18n("Rename Identity"), Ok|Cancel|Help, 
		Ok, true )
{
  QFrame *page = makeMainWidget();
  QGridLayout *glay = new QGridLayout( page, 4, 2, 0, spacingHint() );
  glay->addColSpacing( 1, fontMetrics().maxWidth()*20 );
  glay->setRowStretch( 3, 10 );

  QLabel *label = new QLabel( i18n("Current Name:"), page );
  glay->addWidget( label, 0, 0 );

  mCurrentNameLabel = new QLabel( page );
  glay->addWidget( mCurrentNameLabel, 0, 1 );

  QFont f( mCurrentNameLabel->font() );
  f.setBold(true);
  mCurrentNameLabel->setFont(f);

  glay->addRowSpacing( 1, spacingHint() );

  label = new QLabel( i18n("New Name:"), page );
  glay->addWidget( label, 2, 0 );

  mLineEdit = new QLineEdit( page );
  glay->addWidget( mLineEdit, 2, 1 );
}


void RenameIdentityDialog::showEvent( QShowEvent * )
{
  mLineEdit->setFocus();
}


void RenameIdentityDialog::slotOk( void )
{
  QString identity = identityText().stripWhiteSpace();
  if( identity.isEmpty() == true )
  {
    KMessageBox::error( this, i18n("You must specify an identity") );
    return;
  }

  QStringList::Iterator it;
  for( it = mIdentityList.begin(); it != mIdentityList.end(); ++it ) 
  {
    if( *it == identity )
    {
      KMessageBox::error( this, i18n("The identity already exist") );
      return;
    }
  }

  accept();
}


void RenameIdentityDialog::setIdentities( const QString &current, 
					  const QStringList &list )
{
  mCurrentNameLabel->setText( current );
  mIdentityList = list;
  mLineEdit->setText( current );
  mLineEdit->setSelection( 0, current.length() );
}


QString RenameIdentityDialog::identityText( void )
{
  return( mLineEdit->text() );
}




ConfigureDialog::ConfigureDialog( QWidget *parent, const char *name, 
				  bool modal )
  :KDialogBase( IconList, i18n("Configure"), Help|Default|Apply|Ok|Cancel,
		Ok, parent, name, modal, true )
{
  setHelp( "kmail/kmail.html", QString::null );
  setIconListAllVisible( true );
  enableButton( Default, false ); 

  makeIdentityPage();
  makeNetworkPage();
  makeAppearancePage();
  makeComposerPage();
  makeMimePage();
  makeSecurityPage();
  makeMiscPage();
}


ConfigureDialog::~ConfigureDialog( void )
{
}


void ConfigureDialog::show( void )
{
  if( isVisible() == false )
  {
    setup();
    //showPage(0); Perhaps best to remember the page?
  }
  KDialogBase::show();
}


void ConfigureDialog::makeIdentityPage( void )
{
  QFrame *page = addPage( i18n("Identity"), i18n("Personal information"),
			  QPixmap(user_xpm) );
  QVBoxLayout *topLevel = new QVBoxLayout( page, 0, spacingHint() );
  mIdentity.pageIndex = pageIndex(page);

  QGridLayout *glay = new QGridLayout( topLevel, 12, 3 );
  glay->addColSpacing( 1, fontMetrics().maxWidth()*15 );
  glay->addRowSpacing( 6, spacingHint() );
  glay->setRowStretch( 11, 10 );
  glay->setColStretch( 1, 10 );

  /*
  QLabel *label = new QLabel( i18n("Identity:"), page );
  glay->addWidget( label, 0, 0 );
  QWidget *helper = new QWidget( page );
  glay->addMultiCellWidget( helper, 0, 0, 1, 2 );
  QHBoxLayout *hlay = new QHBoxLayout( helper, 0, spacingHint() );
  mIdentity.identityCombo = new QComboBox( false, helper );
  connect( mIdentity.identityCombo, SIGNAL(activated(int)),
	   this, SLOT(slotIdentitySelectorChanged()) ); 
  hlay->addWidget( mIdentity.identityCombo, 10 );
  QPushButton *newButton = new QPushButton( i18n("New..."), helper );
  connect( newButton, SIGNAL(clicked()),
	   this, SLOT(slotNewIdentity()) );
  newButton->setAutoDefault( false );
  hlay->addWidget( newButton );
  mIdentity.removeIdentityButton = new QPushButton( i18n("Remove"), helper );
  connect( mIdentity.removeIdentityButton, SIGNAL(clicked()),
	   this, SLOT(slotRemoveIdentity()) );
  mIdentity.removeIdentityButton->setAutoDefault( false );
  hlay->addWidget( mIdentity.removeIdentityButton );
  */

  QLabel *label = new QLabel( i18n("Identity:"), page );
  glay->addWidget( label, 0, 0 );
  mIdentity.identityCombo = new QComboBox( false, page );
  connect( mIdentity.identityCombo, SIGNAL(activated(int)),
	   this, SLOT(slotIdentitySelectorChanged()) ); 
  glay->addMultiCellWidget( mIdentity.identityCombo, 0, 0, 1, 2 );

  QWidget *helper = new QWidget( page );
  glay->addMultiCellWidget( helper, 1, 1, 1, 2 );
  QHBoxLayout *hlay = new QHBoxLayout( helper, 0, spacingHint() );
  QPushButton *newButton = new QPushButton( i18n("New..."), helper );
  mIdentity.renameIdentityButton = new QPushButton( i18n("Rename..."), helper);
  mIdentity.removeIdentityButton = new QPushButton( i18n("Remove"), helper );
  newButton->setAutoDefault( false );
  mIdentity.renameIdentityButton->setAutoDefault( false );
  mIdentity.removeIdentityButton->setAutoDefault( false );
  connect( newButton, SIGNAL(clicked()),
	   this, SLOT(slotNewIdentity()) );
  connect( mIdentity.renameIdentityButton, SIGNAL(clicked()),
	   this, SLOT(slotRenameIdentity()) );
  connect( mIdentity.removeIdentityButton, SIGNAL(clicked()),
	   this, SLOT(slotRemoveIdentity()) );
  hlay->addWidget( newButton );
  hlay->addWidget( mIdentity.renameIdentityButton );
  hlay->addWidget( mIdentity.removeIdentityButton );


  label = new QLabel( i18n("Name:"), page );
  glay->addWidget( label, 2, 0 );
  mIdentity.nameEdit = new QLineEdit( page );
  glay->addMultiCellWidget( mIdentity.nameEdit, 2, 2, 1, 2 );

  label = new QLabel( i18n("Organization:"), page );
  glay->addWidget( label, 3, 0 );
  mIdentity.organizationEdit = new QLineEdit( page );
  glay->addMultiCellWidget( mIdentity.organizationEdit, 3, 3, 1, 2 );

  label = new QLabel( i18n("Email Address:"), page );
  glay->addWidget( label, 4, 0 );
  mIdentity.emailEdit = new QLineEdit( page );
  glay->addMultiCellWidget( mIdentity.emailEdit, 4, 4, 1, 2 );

  label = new QLabel( i18n("Reply-To Address:"), page );
  glay->addWidget( label, 5, 0 );
  mIdentity.replytoEdit = new QLineEdit( page );
  glay->addMultiCellWidget( mIdentity.replytoEdit, 5, 5, 1, 2 );  

  QButtonGroup *buttonGroup = new QButtonGroup( page );  
  connect( buttonGroup, SIGNAL(clicked(int)), 
	   this, SLOT(slotSignatureType(int)) ); 
  buttonGroup->hide();
  mIdentity.signatureFileRadio = 
    new QRadioButton( i18n("Use a signature from file"), page );
  buttonGroup->insert( mIdentity.signatureFileRadio );
  glay->addMultiCellWidget( mIdentity.signatureFileRadio, 7, 7, 0, 2 );

  mIdentity.signatureFileLabel = new QLabel( i18n("Signature File:"), page );
  glay->addWidget( mIdentity.signatureFileLabel, 8, 0 );
  mIdentity.signatureFileEdit = new QLineEdit( page );
  connect( mIdentity.signatureFileEdit, SIGNAL(textChanged(const QString &)),
	   this, SLOT( slotSignatureFile(const QString &)) );
  glay->addWidget( mIdentity.signatureFileEdit, 8, 1 );
  mIdentity.signatureBrowseButton = new QPushButton( i18n("Choose..."), page );
  connect( mIdentity.signatureBrowseButton, SIGNAL(clicked()),
	   this, SLOT(slotSignatureChooser()) );
  mIdentity.signatureBrowseButton->setAutoDefault( false );
  glay->addWidget( mIdentity.signatureBrowseButton, 8, 2 );

  mIdentity.signatureExecCheck = 
    new QCheckBox( i18n("The file is a program"), page );
  glay->addWidget( mIdentity.signatureExecCheck, 9, 1 );
  mIdentity.signatureEditButton = new QPushButton( i18n("Edit File"), page );
  connect( mIdentity.signatureEditButton, SIGNAL(clicked()),
	   this, SLOT(slotSignatureEdit()) );
  mIdentity.signatureEditButton->setAutoDefault( false );
  glay->addWidget( mIdentity.signatureEditButton, 9, 2 );

  mIdentity.signatureTextRadio = 
    new QRadioButton( i18n("Specify signature below"), page );
  buttonGroup->insert( mIdentity.signatureTextRadio );
  glay->addMultiCellWidget( mIdentity.signatureTextRadio, 10, 10, 0, 2 );

  mIdentity.signatureTextEdit = new QMultiLineEdit( page );
  mIdentity.signatureTextEdit->setText("Does not work yet");
  glay->addMultiCellWidget( mIdentity.signatureTextEdit, 11, 11, 0, 2 );
}



void ConfigureDialog::makeNetworkPage( void )
{
  QFrame *page = addPage( i18n("Network"),
			  i18n("Setup for sending and receiving messages"),
			  QPixmap(network_xpm) /*UserIcon("network")*/ );
  QVBoxLayout *topLevel = new QVBoxLayout( page, 0, spacingHint() );
  mNetwork.pageIndex = pageIndex(page);

  QTabWidget *tabWidget = new QTabWidget( page, "tab" );
  topLevel->addWidget( tabWidget );
  
  QWidget *page1 = new QWidget( tabWidget );
  tabWidget->addTab( page1, i18n("Sending Mail") );

  QButtonGroup *buttonGroup = new QButtonGroup( page1 );
  buttonGroup->hide();
  connect( buttonGroup, SIGNAL(clicked(int)), 
	   this, SLOT(slotSendmailType(int)) );
 
  QGridLayout *glay = new QGridLayout( page1, 5, 4, spacingHint() );
  glay->addColSpacing( 2, fontMetrics().maxWidth()*15 );

  mNetwork.sendmailRadio = new QRadioButton( i18n("Sendmail"), page1 );
  buttonGroup->insert(mNetwork.sendmailRadio);
  glay->addMultiCellWidget(mNetwork.sendmailRadio, 0, 0, 0, 3);
  QLabel *label = new QLabel( i18n("Location:"), page1 );
  glay->addWidget( label, 1, 1 );
  mNetwork.sendmailLocationEdit = new QLineEdit( page1 );
  glay->addWidget( mNetwork.sendmailLocationEdit, 1, 2 );
  mNetwork.sendmailChooseButton = 
    new QPushButton( i18n("Choose..."), page1 );
  connect( mNetwork.sendmailChooseButton, SIGNAL(clicked()),
	   this, SLOT(slotSendmailChooser()) );
  mNetwork.sendmailChooseButton->setAutoDefault( false );
  glay->addWidget( mNetwork.sendmailChooseButton, 1, 3 );

  mNetwork.smtpRadio = new QRadioButton( i18n("SMTP"), page1 );
  buttonGroup->insert(mNetwork.smtpRadio);
  glay->addMultiCellWidget(mNetwork.smtpRadio, 2, 2, 0, 3); 
  label = new QLabel( i18n("Server:"), page1 );
  glay->addWidget( label, 3, 1 );
  mNetwork.smtpServerEdit = new QLineEdit( page1 );
  glay->addWidget( mNetwork.smtpServerEdit, 3, 2 );
  label = new QLabel( i18n("Port:"), page1 );
  glay->addWidget( label, 4, 1 );
  mNetwork.smtpPortEdit = new QLineEdit( page1 );
  mNetwork.smtpPortEdit->setValidator( new QIntValidator(page1) );
  glay->addWidget( mNetwork.smtpPortEdit, 4, 2 );

  QWidget *page2 = new QWidget( tabWidget );
  tabWidget->addTab( page2, i18n("Properties") );

  glay = new QGridLayout( page2, 4, 3, spacingHint() );
  glay->setColStretch( 2, 10 );

  label = new QLabel( i18n("Default send method:"), page2 );
  glay->addWidget( label, 0, 0 );
  mNetwork.sendMethodCombo = new QComboBox( page2 );
  mNetwork.sendMethodCombo->insertItem(i18n("Send now"));
  mNetwork.sendMethodCombo->insertItem(i18n("Send later"));
  glay->addWidget( mNetwork.sendMethodCombo, 0, 1 );

  label = new QLabel( i18n("Message Property:"), page2 );
  glay->addWidget( label, 1, 0 );
  mNetwork.messagePropertyCombo = new QComboBox( page2 );
  mNetwork.messagePropertyCombo->insertItem(i18n("Allow 8-bit"));
  mNetwork.messagePropertyCombo->insertItem(
    i18n("MIME Compliant (Quoted Printable)"));
  glay->addWidget( mNetwork.messagePropertyCombo, 1, 1 );
  
 label = new QLabel( i18n("Precommand:"), page2 );
  glay->addWidget( label, 2, 0 );
  mNetwork.precommandEdit = new QLineEdit( page2 );
  glay->addWidget( mNetwork.precommandEdit, 2, 1 );

  mNetwork.confirmSendCheck = 
    new QCheckBox(i18n("Confirm before send"), page2 );
  glay->addMultiCellWidget( mNetwork.confirmSendCheck, 3, 3, 0, 1 );



  buttonGroup = new QButtonGroup(i18n("&Incoming Mail"), page );
  topLevel->addWidget(buttonGroup, 10 );

  glay = new QGridLayout( buttonGroup, 6, 2, spacingHint() );
  glay->addColSpacing( 0, fontMetrics().maxWidth()*15 );
  glay->addRowSpacing( 0, fontMetrics().lineSpacing() );
  glay->setColStretch( 0, 10 );
  glay->setRowStretch( 5, 100 );

  label = new QLabel( buttonGroup );
  label->setText(i18n("Accounts:   (add at least one account!)"));
  glay->addMultiCellWidget(label, 1, 1, 0, 1);
  mNetwork.accountList = new ListView( buttonGroup, "accountList", 5 );
  mNetwork.accountList->addColumn( i18n("Name") );
  mNetwork.accountList->addColumn( i18n("Type") );
  mNetwork.accountList->addColumn( i18n("Folder") );
  mNetwork.accountList->setAllColumnsShowFocus( true );
  mNetwork.accountList->setFrameStyle( QFrame::WinPanel + QFrame::Sunken );
  mNetwork.accountList->setSorting( -1 );
  connect( mNetwork.accountList, SIGNAL(selectionChanged ()),
	   this, SLOT(slotAccountSelected()) );
  connect( mNetwork.accountList, SIGNAL(doubleClicked( QListViewItem *)),
	   this, SLOT(slotModifySelectedAccount()) );
  glay->addMultiCellWidget( mNetwork.accountList, 2, 5, 0, 0 );
  
  mNetwork.addAccountButton = 
    new QPushButton( i18n("Add..."), buttonGroup );
  mNetwork.addAccountButton->setAutoDefault( false );
  connect( mNetwork.addAccountButton, SIGNAL(clicked()),
	   this, SLOT(slotAddAccount()) );
  glay->addWidget( mNetwork.addAccountButton, 2, 1 );
  
  mNetwork.modifyAccountButton = 
    new QPushButton( i18n("Modify..."), buttonGroup );
  mNetwork.modifyAccountButton->setAutoDefault( false );
  mNetwork.modifyAccountButton->setEnabled( false );
  connect( mNetwork.modifyAccountButton, SIGNAL(clicked()),
	   this, SLOT(slotModifySelectedAccount()) );
  glay->addWidget( mNetwork.modifyAccountButton, 3, 1 );

  mNetwork.removeAccountButton 
    = new QPushButton( i18n("Remove..."), buttonGroup );
  mNetwork.removeAccountButton->setAutoDefault( false );
  mNetwork.removeAccountButton->setEnabled( false );
  connect( mNetwork.removeAccountButton, SIGNAL(clicked()),
	   this, SLOT(slotRemoveSelectedAccount()) );
  glay->addWidget( mNetwork.removeAccountButton, 4, 1 );
}



void ConfigureDialog::makeAppearancePage( void )
{
  QVBox *vbox = addVBoxPage( i18n("Appearance"), 
			     i18n("Customize visual appearance"),
			     QPixmap(appearance_xpm)); 
  QTabWidget *tabWidget = new QTabWidget( vbox, "tab" );
  mAppearance.pageIndex = pageIndex(vbox);
  
  QWidget *page1 = new QWidget( tabWidget );
  tabWidget->addTab( page1, i18n("Fonts") );
  QVBoxLayout *vlay = new QVBoxLayout( page1, spacingHint() );
  mAppearance.customFontCheck = 
    new QCheckBox( i18n("Use custom fonts"), page1 );
  connect( mAppearance.customFontCheck, SIGNAL(clicked() ),
	   this, SLOT(slotCustomFontSelectionChanged()) );
  vlay->addWidget( mAppearance.customFontCheck );
  QFrame *hline = new QFrame( page1 );
  hline->setFrameStyle( QFrame::Sunken | QFrame::HLine );
  vlay->addWidget( hline );
  QHBoxLayout *hlay = new QHBoxLayout( vlay );
  mAppearance.fontLocationLabel = new QLabel( i18n("Location:"), page1 );
  hlay->addWidget( mAppearance.fontLocationLabel );
  mAppearance.fontLocationCombo = new QComboBox( page1 );
  //
  // If you add or remove entries to this list, make sure to revise 
  // slotFontSelectorChanged(..) as well.
  //
  QStringList fontStringList;
  fontStringList.append( i18n("Message Body") );
  fontStringList.append( i18n("Message List") );
  fontStringList.append( i18n("Folder List") );
  fontStringList.append( i18n("Quoted text - First level") );
  fontStringList.append( i18n("Quoted text - Second level") );
  fontStringList.append( i18n("Quoted text - Third level") );
  mAppearance.fontLocationCombo->insertStringList(fontStringList);

  connect( mAppearance.fontLocationCombo, SIGNAL(activated(int) ),
	   this, SLOT(slotFontSelectorChanged(int)) );
  hlay->addWidget( mAppearance.fontLocationCombo );
  hlay->addStretch(10);
  vlay->addSpacing( spacingHint() );
  mAppearance.fontChooser = 
    new KFontChooser( page1, "font", false, QStringList(), false, 4 );
  vlay->addWidget( mAppearance.fontChooser );

  QWidget *page2 = new QWidget( tabWidget );
  tabWidget->addTab( page2, i18n("Colors") );
  vlay = new QVBoxLayout( page2, spacingHint() );
  mAppearance.customColorCheck = 
    new QCheckBox( i18n("Use custom colors"), page2 );
  connect( mAppearance.customColorCheck, SIGNAL(clicked() ),
	   this, SLOT(slotCustomColorSelectionChanged()) );
  vlay->addWidget( mAppearance.customColorCheck );
  hline = new QFrame( page2 );
  hline->setFrameStyle( QFrame::Sunken | QFrame::HLine );
  vlay->addWidget( hline );

  QStringList modeList;
  modeList.append( i18n("Composer Background") );
  modeList.append( i18n("Normal Text") );
  modeList.append( i18n("Quoted Text - First level") );
  modeList.append( i18n("Quoted Text - Second level") );
  modeList.append( i18n("Quoted Text - Third level") );
  modeList.append( i18n("URL Link") );
  modeList.append( i18n("Followed URL Link") );
  modeList.append( i18n("New Message") );
  modeList.append( i18n("Unread Message") );

  mAppearance.colorList = new ColorListBox( page2 );
  vlay->addWidget( mAppearance.colorList, 10 );
  for( uint i=0; i<modeList.count(); i++ )
  {
    ColorListItem *listItem = new ColorListItem( modeList[i] );
    mAppearance.colorList->insertItem( listItem );
  }

  mAppearance.recycleColorCheck = 
    new QCheckBox( i18n("Recycle colors on deep quoting"), page2 );
  vlay->addWidget( mAppearance.recycleColorCheck );


  QWidget *page3 = new QWidget( tabWidget );
  tabWidget->addTab( page3, i18n("Layout") );
  vlay = new QVBoxLayout( page3, spacingHint() );

  mAppearance.longFolderCheck = 
    new QCheckBox( i18n("Show long folder list"), page3 );
  vlay->addWidget( mAppearance.longFolderCheck );

  mAppearance.messageSizeCheck = 
    new QCheckBox( i18n("Display message sizes"), page3 );
  vlay->addWidget( mAppearance.messageSizeCheck );

  mAppearance.nestedMessagesCheck = 
    new QCheckBox( i18n("Thread list of message headers"), page3 );
  vlay->addWidget( mAppearance.nestedMessagesCheck );

  QButtonGroup *group = new QButtonGroup( i18n("HTML"), page3 );
  vlay->addWidget( group );
  QVBoxLayout *vlay2 = new QVBoxLayout( group, spacingHint() );
  vlay2->addSpacing( fontMetrics().lineSpacing() );
  mAppearance.htmlMailCheck = 
    new QCheckBox( i18n("Prefer plain text to HTML rendering"), group );
  vlay2->addWidget( mAppearance.htmlMailCheck );
  QLabel *label = new QLabel( group );
  label->setAlignment( WordBreak);
  label->setText(i18n("Note that HTML rendering introduces security implications."));
  vlay2->addWidget( label );

  /*
  mAppearance.htmlMailCheck = 
    new QCheckBox( i18n("Prefer plain text to HTML rendering"), page3 );
  vlay->addWidget( mAppearance.htmlMailCheck );
  */

  vlay->addStretch(10); // Eat unused space a bottom


  QWidget *page4 = new QWidget( tabWidget );
  tabWidget->addTab( page4, i18n("Profiles") );
  vlay = new QVBoxLayout( page4, spacingHint() );
  
  label = new QLabel( page4 );
  label->setText(i18n("Define or use a GUI profile"));
  vlay->addWidget( label );

  mAppearance.profileList = new ListView( page4, "tagList" );
  mAppearance.profileList->addColumn( i18n("Available profiles") );
  mAppearance.profileList->setAllColumnsShowFocus( true );
  mAppearance.profileList->setFrameStyle( QFrame::WinPanel + QFrame::Sunken );
  mAppearance.profileList->setSorting( -1 );
  vlay->addWidget( mAppearance.profileList, 1 );

  hlay = new QHBoxLayout( vlay );
  QPushButton *pushButton = new QPushButton(i18n("&New"), page4 );
  pushButton->setAutoDefault( false );
  hlay->addWidget( pushButton );
  mAppearance.profileDeleteButton = new QPushButton(i18n("Dele&te"), page4 );
  mAppearance.profileDeleteButton->setAutoDefault( false );
  hlay->addWidget( mAppearance.profileDeleteButton );
  hlay->addStretch(10);

  mAppearance.mListItemDefault = 
    new QListViewItem( mAppearance.profileList, 
    i18n("KMail Classic - KMail as you know it") );
  mAppearance.mListItemNewFeature = 
    new QListViewItem( mAppearance.profileList, mAppearance.mListItemDefault, 
    i18n("New Features - Extended functionality in KDE-2") );
  mAppearance.mListItemContrast = 
    new QListViewItem(mAppearance.profileList, mAppearance.mListItemNewFeature,
    i18n("High Contrast - For the visually impaired user"));
  mAppearance.profileList->setSelected( mAppearance.mListItemDefault, true );

     
}





void ConfigureDialog::makeComposerPage( void )
{
  QFrame *page = addPage( i18n("Composer"),
			  i18n("Phrases and general behavior"),
			  QPixmap(composer_xpm) );
  QVBoxLayout *topLevel = new QVBoxLayout( page, 0, spacingHint() );
  mComposer.pageIndex = pageIndex(page);

  QGroupBox *group = new QGroupBox(i18n("Phrases"), page );
  topLevel->addWidget( group );

  QGridLayout *glay = new QGridLayout( group, 6, 2, spacingHint() );
  glay->addRowSpacing( 0, fontMetrics().lineSpacing() );
  glay->setColStretch( 1, 10 );

  QLabel *label = new QLabel( group );
  label->setText( 
     i18n( "The following placeholders are supported in the reply phrases:\n"
	   "%D=date, %S=subject, %F=sender, %%=percent sign, %_=space"));
  glay->addMultiCellWidget( label, 1, 1, 0, 1 );
  label = new QLabel( i18n("Reply to sender:"), group );
  glay->addWidget( label, 2, 0 );
  mComposer.phraseReplyEdit = new QLineEdit( group );
  glay->addWidget( mComposer.phraseReplyEdit, 2, 1 );
  label = new QLabel( i18n("Reply to all:"), group );
  glay->addWidget( label, 3, 0 );
  mComposer.phraseReplyAllEdit = new QLineEdit( group );
  glay->addWidget( mComposer.phraseReplyAllEdit, 3, 1 );
  label = new QLabel( i18n("Forward:"), group );
  glay->addWidget( label, 4, 0 );
  mComposer.phraseForwardEdit = new QLineEdit( group );
  glay->addWidget( mComposer.phraseForwardEdit, 4, 1 );
  label = new QLabel( i18n("Indentation:"), group );
  glay->addWidget( label, 5, 0 );
  mComposer.phraseindentPrefixEdit = new QLineEdit( group );
  glay->addWidget( mComposer.phraseindentPrefixEdit, 5, 1 );

  mComposer.autoAppSignFileCheck =
    new QCheckBox( i18n("Automatically append signature"), page );
  topLevel->addWidget( mComposer.autoAppSignFileCheck );

  mComposer.smartQuoteCheck =
    new QCheckBox( i18n("Use smart quoting"), page );
  topLevel->addWidget( mComposer.smartQuoteCheck );

  mComposer.pgpAutoSignatureCheck =
    new QCheckBox( i18n("Automatically sign messages using PGP"), page );
  topLevel->addWidget( mComposer.pgpAutoSignatureCheck );

  QHBoxLayout *hlay = new QHBoxLayout( topLevel );
  mComposer.wordWrapCheck =
    new QCheckBox( i18n("Word wrap at column:"), page );
  connect( mComposer.wordWrapCheck, SIGNAL(clicked() ),
	   this, SLOT(slotWordWrapSelectionChanged()) );
  hlay->addWidget( mComposer.wordWrapCheck );
  mComposer.wrapColumnSpin = new QSpinBox( page );
  mComposer.wrapColumnSpin->setRange( 1, 10000 );
  hlay->addWidget( mComposer.wrapColumnSpin, 0, AlignLeft );
  hlay->addStretch(10);

  topLevel->addStretch(10);
}



void ConfigureDialog::makeMimePage( void )
{
  QFrame *page = addPage( i18n("Mime Headers"),
    i18n("Define custom mime header tags for outgoing emails"),
			  QPixmap(mime_xpm) );
  QVBoxLayout *topLevel = new QVBoxLayout( page, 0, spacingHint() );
  mMime.pageIndex = pageIndex(page);
  
  QLabel *label = new QLabel( page );
  label->setText(i18n("Define custom mime header tags for outgoing emails:"));
  topLevel->addWidget( label );

  mMime.tagList = new ListView( page, "tagList" );
  mMime.tagList->addColumn( i18n("Name") );
  mMime.tagList->addColumn( i18n("Value") );
  mMime.tagList->setAllColumnsShowFocus( true );
  mMime.tagList->setFrameStyle( QFrame::WinPanel + QFrame::Sunken );
  mMime.tagList->setSorting( -1 );
  connect( mMime.tagList, SIGNAL(selectionChanged()),
	   this, SLOT(slotMimeHeaderSelectionChanged()) );
  topLevel->addWidget( mMime.tagList );

  QGridLayout *glay = new QGridLayout( topLevel, 3, 2 );
  glay->setColStretch( 1, 10 );

  mMime.tagNameLabel = new QLabel(i18n("Name:"), page );
  mMime.tagNameLabel->setEnabled(false);
  glay->addWidget( mMime.tagNameLabel, 0, 0 );
  mMime.tagNameEdit = new QLineEdit(page);
  mMime.tagNameEdit->setEnabled(false);
  connect( mMime.tagNameEdit, SIGNAL(textChanged(const QString&)),
	   this, SLOT(slotMimeHeaderNameChanged(const QString&)) );
  glay->addWidget( mMime.tagNameEdit, 0, 1 );
  
  mMime.tagValueLabel = new QLabel(i18n("Value:"), page );
  mMime.tagValueLabel->setEnabled(false);
  glay->addWidget( mMime.tagValueLabel, 1, 0 );  
  mMime.tagValueEdit = new QLineEdit(page);
  mMime.tagValueEdit->setEnabled(false);
  connect( mMime.tagValueEdit, SIGNAL(textChanged(const QString&)),
	   this, SLOT(slotMimeHeaderValueChanged(const QString&)) );
  glay->addWidget( mMime.tagValueEdit, 1, 1 );

  QWidget *helper = new QWidget( page );
  glay->addWidget( helper, 2, 1 );
  QHBoxLayout *hlay = new QHBoxLayout( helper, 0, spacingHint() );
  QPushButton *pushButton = new QPushButton(i18n("&New"), helper );
  connect( pushButton, SIGNAL(clicked()), this, SLOT(slotNewMimeHeader()) );
  pushButton->setAutoDefault( false );
  hlay->addWidget( pushButton );
  pushButton = new QPushButton(i18n("Dele&te"), helper );
  connect( pushButton, SIGNAL(clicked()), this, SLOT(slotDeleteMimeHeader()));
  pushButton->setAutoDefault( false );
  hlay->addWidget( pushButton );
  hlay->addStretch(10);

  topLevel->addSpacing( spacingHint()*2 );
}


void ConfigureDialog::makeSecurityPage( void )
{
  QVBox *vbox = addVBoxPage( i18n("Security"), 
			     i18n("Security Settings"),
			     QPixmap(security_xpm) /*UserIcon("security")*/ );
  mSecurity.pageIndex = pageIndex(vbox);

  QTabWidget *tabWidget = new QTabWidget( vbox, "tab" );
  QWidget *page1 = new QWidget( tabWidget );
  tabWidget->addTab( page1, i18n("PGP") );
  QVBoxLayout *vlay = new QVBoxLayout( page1, spacingHint() );

  mSecurity.pgpConfig = new KpgpConfig(page1);
  vlay->addWidget( mSecurity.pgpConfig );
  vlay->addStretch(10);
}

#include <kinstance.h>
#include <kglobal.h>

void ConfigureDialog::makeMiscPage( void )
{
  //KIconLoader *loader = instace->iconLoader();
  ///KGlobal::instance()->iconLoader()

  QFrame *page = addPage( i18n("Miscellaneous"), i18n("Various settings"),
    KGlobal::instance()->iconLoader()->loadIcon( "misc", KIcon::NoGroup, 
    KIcon::SizeMedium ));
  QVBoxLayout *topLevel = new QVBoxLayout( page, 0, spacingHint() );
  mMisc.pageIndex = pageIndex(page);

  //---------- group: folders
  QGroupBox *group = new QGroupBox( i18n("&Folders"), page );
  topLevel->addWidget( group );
  QVBoxLayout *vlay = new QVBoxLayout( group, spacingHint() );
  vlay->addSpacing( fontMetrics().lineSpacing() );
  mMisc.emptyTrashCheck =
    new QCheckBox(i18n("Empty trash on exit"), group );
  vlay->addWidget( mMisc.emptyTrashCheck );
  mMisc.sendOutboxCheck =
    new QCheckBox(i18n("Send Mail in outbox Folder on Check"), group );
  vlay->addWidget( mMisc.sendOutboxCheck );
  mMisc.sendReceiptCheck = new QCheckBox( 
    i18n("Automatically send receive- and read confirmations"), group );
  vlay->addWidget( mMisc.sendReceiptCheck );
  mMisc.compactOnExitCheck =
    new QCheckBox(i18n("Compact all folders on exit"), group );
  vlay->addWidget( mMisc.compactOnExitCheck );

  //---------- group: External editor
  group = new QGroupBox( i18n("&External Editor"), page );
  topLevel->addWidget( group );
  vlay = new QVBoxLayout( group, spacingHint() );
  vlay->addSpacing( fontMetrics().lineSpacing() );
  mMisc.externalEditorCheck = 
    new QCheckBox(i18n("Use external editor instead of composer"), group );
  connect( mMisc.externalEditorCheck, SIGNAL(clicked() ),
	   this, SLOT(slotExternalEditorSelectionChanged()) );
  vlay->addWidget( mMisc.externalEditorCheck );
  QHBoxLayout *hlay = new QHBoxLayout( vlay );
  mMisc.externalEditorLabel = new QLabel( i18n("Specify editor:"), group );
  hlay->addWidget( mMisc.externalEditorLabel );
  mMisc.externalEditorEdit = new QLineEdit( group );
  hlay->addWidget( mMisc.externalEditorEdit );
  mMisc.externalEditorChooseButton = 
    new QPushButton( i18n("Choose..."), group );
  connect( mMisc.externalEditorChooseButton, SIGNAL(clicked()),
	   this, SLOT(slotExternalEditorChooser()) );
  mMisc.externalEditorChooseButton->setAutoDefault( false );
  hlay->addWidget( mMisc.externalEditorChooseButton );
  mMisc.externalEditorHelp = new QLabel( group );
  mMisc.externalEditorHelp->setText(
    i18n("\"%f\" will be replaced with the filename to edit."));
  vlay->addWidget( mMisc.externalEditorHelp );

  //---------- group: New Mail Notification
  group = new QGroupBox( i18n("&New Mail Notification"), page );
  topLevel->addWidget( group );
  vlay = new QVBoxLayout( group, spacingHint() );
  vlay->addSpacing( fontMetrics().lineSpacing() );
  mMisc.beepNewMailCheck =
    new QCheckBox(i18n("Beep on new mail"), group );
  vlay->addWidget( mMisc.beepNewMailCheck );
  mMisc.showMessageBoxCheck =
    new QCheckBox(i18n("Display message box on new mail"), group );
  vlay->addWidget( mMisc.showMessageBoxCheck );
  mMisc.mailCommandCheck =
    new QCheckBox( i18n("Execute command line on new mail"), group );
  vlay->addWidget( mMisc.mailCommandCheck );
  connect( mMisc.mailCommandCheck, SIGNAL(clicked() ),
	   this, SLOT(slotMailCommandSelectionChanged()) );
  hlay = new QHBoxLayout( vlay );
  mMisc.mailCommandLabel = new QLabel( i18n("Specify command:"), group );
  hlay->addWidget( mMisc.mailCommandLabel );
  mMisc.mailCommandEdit = new QLineEdit( group );
  hlay->addWidget( mMisc.mailCommandEdit );
  mMisc.mailCommandChooseButton = 
    new QPushButton( i18n("Choose..."), group );
  connect( mMisc.mailCommandChooseButton, SIGNAL(clicked()),
	   this, SLOT(slotMailCommandChooser()) );
  mMisc.mailCommandChooseButton->setAutoDefault( false );
  hlay->addWidget( mMisc.mailCommandChooseButton );

  int w1 = mMisc.externalEditorLabel->sizeHint().width();
  int w2 = mMisc.mailCommandLabel->sizeHint().width();
  mMisc.externalEditorLabel->setMinimumWidth( QMAX( w1, w2 ) );
  mMisc.mailCommandLabel->setMinimumWidth( QMAX( w1, w2 ) );

  topLevel->addStretch( 10 );
}



void ConfigureDialog::setup( void )
{
  setupIdentityPage();
  setupNetworkPage();
  setupAppearancePage();
  setupComposerPage();
  setupMimePage();
  setupSecurityPage();
  setupMiscPage();

}



void ConfigureDialog::setupIdentityPage( void )
{
  mIdentityList.importData();
  mIdentity.identityCombo->clear();
  mIdentity.identityCombo->insertStringList( mIdentityList.identities() );
  slotIdentitySelectorChanged(); // This will trigger an update
}


void ConfigureDialog::setupNetworkPage( void )
{
  if( kernel->msgSender()->method() == KMSender::smMail )
  {
    mNetwork.sendmailRadio->setChecked(true);
    slotSendmailType(0);
  }
  else if( kernel->msgSender()->method() == KMSender::smSMTP )
  {
    mNetwork.smtpRadio->setChecked(true);
    slotSendmailType(1);
  }
  
  mNetwork.sendmailLocationEdit->setText( kernel->msgSender()->mailer() );
  mNetwork.smtpServerEdit->setText( kernel->msgSender()->smtpHost() );
  mNetwork.smtpPortEdit->setText( 
    QString().setNum(kernel->msgSender()->smtpPort()) );
 mNetwork.precommandEdit->setText( kernel->msgSender()->precommand() );

  KConfig &config = *kapp->config();
  config.setGroup("Composer");

  mNetwork.sendMethodCombo->setCurrentItem( 
    kernel->msgSender()->sendImmediate() ? 0 : 1 ); 
  mNetwork.messagePropertyCombo->setCurrentItem( 
    kernel->msgSender()->sendQuotedPrintable() ? 1 : 0 );
  mNetwork.confirmSendCheck->setChecked( 
    config.readBoolEntry( "confirm-before-send", false ) );

  mNetwork.accountList->clear();
  QListViewItem *top = 0;
  for( KMAccount *a = kernel->acctMgr()->first(); a!=0; 
       a = kernel->acctMgr()->next() )
  {
    QListViewItem *listItem = 
      new QListViewItem( mNetwork.accountList, top, a->name(), a->type() );
    if( a->folder() ) 
    {
      listItem->setText( 2, a->folder()->name() );
    }
    top = listItem;
  }

  QListViewItem *listItem = mNetwork.accountList->firstChild();
  if( listItem != 0 )
  {
    mNetwork.accountList->setSelected( listItem, true );  
  }
}

void ConfigureDialog::setupAppearancePage( void )
{
  KConfig &config = *kapp->config();
  config.setGroup("Fonts");

  mAppearance.fontString[0] = 
    config.readEntry("body-font", "helvetica-medium-r-12");
  mAppearance.fontString[1] = 
    config.readEntry("list-font", "helvetica-medium-r-12");
  mAppearance.fontString[2] = 
    config.readEntry("folder-font", "helvetica-medium-r-12");
  mAppearance.fontString[3] = 
    config.readEntry("quote1-font", "helvetica-medium-r-12");
  mAppearance.fontString[4] = 
    config.readEntry("quote2-font", "helvetica-medium-r-12");
  mAppearance.fontString[5] = 
    config.readEntry("quote3-font", "helvetica-medium-r-12");

  bool state = config.readBoolEntry("defaultFonts", false );
  mAppearance.customFontCheck->setChecked( state == false ? true : false );
  slotCustomFontSelectionChanged();
  updateFontSelector();

  config.setGroup("Reader");

  QColor defaultColor = QColor(kapp->palette().normal().base());  
  mAppearance.colorList->setColor(
    0, config.readColorEntry("BackgroundColor",&defaultColor ) );

  defaultColor = QColor(kapp->palette().normal().text());
  mAppearance.colorList->setColor( 
    1, config.readColorEntry("ForegroundColor",&defaultColor ) );
  
  defaultColor = QColor(kapp->palette().normal().text());
  mAppearance.colorList->setColor( 
    2, config.readColorEntry("QuoutedText1",&defaultColor ) );

  defaultColor = QColor(kapp->palette().normal().text());
  mAppearance.colorList->setColor( 
    3, config.readColorEntry("QuoutedText2",&defaultColor ) );

  defaultColor = QColor(kapp->palette().normal().text());
  mAppearance.colorList->setColor( 
    4, config.readColorEntry("QuoutedText3",&defaultColor ) );

  defaultColor = KGlobalSettings::linkColor();
  mAppearance.colorList->setColor( 
    5, config.readColorEntry("LinkColor",&defaultColor ) );

  defaultColor = KGlobalSettings::visitedLinkColor();
  mAppearance.colorList->setColor(
    6, config.readColorEntry("FollowedColor",&defaultColor ) );

  defaultColor = QColor("blue");
  mAppearance.colorList->setColor( 
    7, config.readColorEntry("NewMessage",&defaultColor ) );

  defaultColor = QColor("red");
  mAppearance.colorList->setColor(
    8, config.readColorEntry("UnreadMessage",&defaultColor ) );

  state = config.readBoolEntry("defaultColors", true );
  mAppearance.customColorCheck->setChecked( state == false ? true : false );
  slotCustomColorSelectionChanged();

  state = config.readBoolEntry( "RecycleQuoteColors", false );
  mAppearance.recycleColorCheck->setChecked( state );

  config.setGroup("Geometry");
  state = config.readBoolEntry( "longFolderList", false );
  mAppearance.longFolderCheck->setChecked( state );

  state = config.readBoolEntry( "nestedMessages", false );
  mAppearance.nestedMessagesCheck->setChecked( state );

  config.setGroup("Reader");
  state = config.readBoolEntry( "htmlMail", true );
  mAppearance.htmlMailCheck->setChecked( !state );

  config.setGroup("General");
  state = config.readBoolEntry( "showMessageSize", false );
  mAppearance.messageSizeCheck->setChecked( state );

}


void ConfigureDialog::setupComposerPage( void )
{
  KConfig &config = *kapp->config();
  config.setGroup("KMMessage");

  QString str = config.readEntry("phrase-reply");
  if (str.isEmpty()) str = i18n("On %D, you wrote:");
  mComposer.phraseReplyEdit->setText(str);

  str = config.readEntry("phrase-reply-all");
  if (str.isEmpty()) str = i18n("On %D, %F wrote:");
  mComposer.phraseReplyAllEdit->setText(str);

  str = config.readEntry("phrase-forward");
  if (str.isEmpty()) str = i18n("Forwarded Message");
  mComposer.phraseForwardEdit->setText(str);

  str = config.readEntry("indent-prefix", ">%_"); 
  mComposer.phraseindentPrefixEdit->setText(str );

  config.setGroup("Composer");

  bool state = stricmp( config.readEntry("signature"), "auto" ) == 0;
  mComposer.autoAppSignFileCheck->setChecked( state );

  state = config.readBoolEntry( "smart-quote", true );
  mComposer.smartQuoteCheck->setChecked(state);

  state = config.readBoolEntry( "pgp-auto-sign", false );
  mComposer.pgpAutoSignatureCheck->setChecked(state);

  state = config.readBoolEntry( "word-wrap", true );
  mComposer.wordWrapCheck->setChecked( state );
  
  int value = config.readEntry("break-at","78" ).toInt();
  mComposer.wrapColumnSpin->setValue( mComposer.wrapColumnSpin->bound(value) );
  slotWordWrapSelectionChanged(); 
}

void ConfigureDialog::setupMimePage( void )
{
  KConfig &config = *kapp->config();
  config.setGroup("General");

  mMime.tagList->clear();
  QListViewItem *top = 0;

  int count = config.readNumEntry( "mime-header-count", 0 );
  mMime.tagList->clear();
  for(int i = 0; i < count; i++) 
  {
    config.setGroup( QString("Mime #%1").arg(i) );
    QString name  = config.readEntry("name", "");
    QString value = config.readEntry("value", "");
    if( name.length() > 0 ) 
    {
      QListViewItem *listItem = 
	new QListViewItem( mMime.tagList, top, name, value );
      top = listItem;
    }
  }

}

void ConfigureDialog::setupSecurityPage( void )
{
  // Nothing here
}


void ConfigureDialog::setupMiscPage( void )
{
  KConfig &config = *kapp->config();
  config.setGroup("General");

  bool state = config.readBoolEntry("empty-trash-on-exit",true);
  mMisc.emptyTrashCheck->setChecked( state );
  state = config.readBoolEntry("sendOnCheck", false);
  mMisc.sendOutboxCheck->setChecked( state );
  state = config.readBoolEntry("send-receipts", false );
  mMisc.sendReceiptCheck->setChecked( state );
  state = config.readBoolEntry("compact-all-on-exit", true );
  mMisc.compactOnExitCheck->setChecked( state );
  state = config.readBoolEntry( "use-external-editor", false );
  mMisc.externalEditorCheck->setChecked( state );
  mMisc.externalEditorEdit->setText( config.readEntry("external-editor", "") );
  state = config.readBoolEntry("beep-on-mail", false );
  mMisc.beepNewMailCheck->setChecked( state );
  state = config.readBoolEntry("msgbox-on-mail", false);
  mMisc.showMessageBoxCheck->setChecked( state );
  state = config.readBoolEntry("exec-on-mail", false);
  mMisc.mailCommandCheck->setChecked( state );
  mMisc.mailCommandEdit->setText( config.readEntry("exec-on-mail-cmd", ""));
  slotExternalEditorSelectionChanged();
  slotMailCommandSelectionChanged();
}


void ConfigureDialog::installProfile( void )
{
  QListViewItem *item = mAppearance.profileList->selectedItem();
  if( item == 0 )
  {
    return;
  }

  if( item == mAppearance.mListItemDefault )
  {
    mAppearance.fontString[0] = "adobe-normal-r-12";
    mAppearance.fontString[1] = "adobe-normal-r-12";
    mAppearance.fontString[2] = "adobe-normal-r-12";
    mAppearance.fontString[3] = "adobe-normal-i-12"; 
    mAppearance.fontString[4] = "adobe-normal-i-12";
    mAppearance.fontString[5] = "adobe-normal-i-12"; 
    mAppearance.customFontCheck->setChecked( true );
    mAppearance.colorList->setColor( 0, kapp->palette().normal().base() );
    mAppearance.colorList->setColor( 1, kapp->palette().normal().text() );
    mAppearance.colorList->setColor( 2, kapp->palette().normal().text() );
    mAppearance.colorList->setColor( 3, kapp->palette().normal().text() );
    mAppearance.colorList->setColor( 4, kapp->palette().normal().text() );
    mAppearance.colorList->setColor( 5, KGlobalSettings::linkColor() );
    mAppearance.colorList->setColor( 6, KGlobalSettings::visitedLinkColor() );
    mAppearance.colorList->setColor( 7, blue );
    mAppearance.colorList->setColor( 8, red );
    mAppearance.customColorCheck->setChecked( true );
    
    mAppearance.longFolderCheck->setChecked( false );
    mAppearance.messageSizeCheck->setChecked( false );
    mAppearance.nestedMessagesCheck->setChecked( false );
    mAppearance.htmlMailCheck->setChecked( true );
  }
  else if( item == mAppearance.mListItemNewFeature )
  {
    mAppearance.fontString[0] = "adobe-normal-r-12";
    mAppearance.fontString[1] = "adobe-normal-r-12";
    mAppearance.fontString[2] = "adobe-normal-r-12";
    mAppearance.fontString[3] = "adobe-normal-r-12";
    mAppearance.fontString[4] = "adobe-normal-r-12";
    mAppearance.fontString[5] = "adobe-normal-r-12";
    mAppearance.customFontCheck->setChecked( true );
    mAppearance.colorList->setColor( 0, kapp->palette().normal().base() );
    mAppearance.colorList->setColor( 1, kapp->palette().normal().text() );
    mAppearance.colorList->setColor( 2, red );
    mAppearance.colorList->setColor( 3, QColor("#006400") );
    mAppearance.colorList->setColor( 4, QColor("#832B8B") );
    mAppearance.colorList->setColor( 5, blue );
    mAppearance.colorList->setColor( 6, red );
    mAppearance.colorList->setColor( 7, blue );
    mAppearance.colorList->setColor( 8, red );
    mAppearance.customColorCheck->setChecked( true );
    
    mAppearance.longFolderCheck->setChecked( false );
    mAppearance.messageSizeCheck->setChecked( false );
    mAppearance.nestedMessagesCheck->setChecked( true );
    mAppearance.htmlMailCheck->setChecked( false );
  }
  else if( item == mAppearance.mListItemContrast )
  {
    mAppearance.fontString[0] = "adobe-bold-r-14";
    mAppearance.fontString[1] = "adobe-bold-r-14";
    mAppearance.fontString[2] = "adobe-bold-r-14";
    mAppearance.fontString[3] = "adobe-bold-r-14"; 
    mAppearance.fontString[4] = "adobe-bold-r-14";
    mAppearance.fontString[5] = "adobe-bold-r-14";
    mAppearance.customFontCheck->setChecked( true );
    mAppearance.colorList->setColor( 0, QColor("#FAEBD7") );
    mAppearance.colorList->setColor( 1, black );
    mAppearance.colorList->setColor( 2, red );
    mAppearance.colorList->setColor( 3, QColor("#006400") );
    mAppearance.colorList->setColor( 4, QColor("#832B8B") );
    mAppearance.colorList->setColor( 5, blue );
    mAppearance.colorList->setColor( 6, red );
    mAppearance.colorList->setColor( 7, blue );
    mAppearance.colorList->setColor( 8, red );
    mAppearance.customColorCheck->setChecked( true );
    
    mAppearance.longFolderCheck->setChecked( false );
    mAppearance.messageSizeCheck->setChecked( false );
    mAppearance.nestedMessagesCheck->setChecked( true );
    mAppearance.htmlMailCheck->setChecked( false );
  }
  else
  {
  }

  slotCustomFontSelectionChanged();
  updateFontSelector();
  slotCustomColorSelectionChanged();
}


//
// Refresh the font selector with the active font string. The current
// font selector setting is ignored.
//
void ConfigureDialog::updateFontSelector( void )
{
  mAppearance.activeFontIndex = mAppearance.fontLocationCombo->currentItem();
  if( mAppearance.activeFontIndex < 0 ) mAppearance.activeFontIndex = 0;

  int i=mAppearance.activeFontIndex;
  if( mAppearance.fontString[i].isEmpty() == false )
  {
    mAppearance.fontChooser->setFont( kstrToFont(mAppearance.fontString[i] ) );
  }
}



void ConfigureDialog::slotDefault( void )
{
  KMessageBox::sorry( this, i18n( "This feature is not working yet." ) );
}


void ConfigureDialog::slotOk( void )
{
  slotApply();
  accept();
}


void ConfigureDialog::slotApply( void )
{
  KConfig &config = *kapp->config();

  int activePage = activePageIndex();
  if( activePage == mIdentity.pageIndex )
  {
    saveActiveIdentity(); // Copy from textfields into list
    mIdentityList.exportData();
  }
  else if( activePage == mNetwork.pageIndex )
  {
    // Sending mail
    if( mNetwork.sendmailRadio->isChecked() )
    {
      kernel->msgSender()->setMethod( KMSender::smMail );
    }
    else
    {
      kernel->msgSender()->setMethod( KMSender::smSMTP );
    }  
    kernel->msgSender()->setMailer(mNetwork.sendmailLocationEdit->text() );
    kernel->msgSender()->setSmtpHost( mNetwork.smtpServerEdit->text() );
    kernel->msgSender()->setSmtpPort( mNetwork.smtpPortEdit->text().toInt() );
    kernel->msgSender()->setPrecommand( mNetwork.precommandEdit->text() );

    bool sendNow = mNetwork.sendMethodCombo->currentItem() == 0;
    kernel->msgSender()->setSendImmediate( sendNow );
    bool quotedPrintable = mNetwork.messagePropertyCombo->currentItem() == 1;
    kernel->msgSender()->setSendQuotedPrintable( quotedPrintable );
    kernel->msgSender()->writeConfig(FALSE);
    // Moved from composer page !
    config.setGroup("Composer");
    bool confirmBeforeSend = mNetwork.confirmSendCheck->isChecked();
    config.writeEntry("confirm-before-send", confirmBeforeSend );

    // Incoming mail
    kernel->acctMgr()->writeConfig(FALSE);
  }
  else if( activePage == mAppearance.pageIndex )
  {
    //
    // Fake a selector change. It will save the current selector setting
    // into the font string with index "mAppearance.activeFontIndex"
    //
    slotFontSelectorChanged( mAppearance.activeFontIndex );

    //
    // If the profile tab page is visible, then install the selected
    // entry. It will the be written to disk below.
    //
    if( mAppearance.profileList->isVisible() )
    {
      installProfile();
    }

    config.setGroup("Fonts");
    bool defaultFonts = !mAppearance.customFontCheck->isChecked();
    config.writeEntry("defaultFonts", defaultFonts );
    config.writeEntry( "body-font",   mAppearance.fontString[0] );
    config.writeEntry( "list-font",   mAppearance.fontString[1] );
    config.writeEntry( "folder-font", mAppearance.fontString[2] );
    config.writeEntry( "quote1-font", mAppearance.fontString[3] );
    config.writeEntry( "quote2-font", mAppearance.fontString[4] );
    config.writeEntry( "quote3-font", mAppearance.fontString[5] );
//  GS - should this be here?
//    printf("WRITE: %s\n", mAppearance.fontString[3].latin1() );

    config.setGroup("Reader");
    bool defaultColors = !mAppearance.customColorCheck->isChecked();
    config.writeEntry("defaultColors", defaultColors );
    if (!defaultColors)
    {
       // Don't write color info when we use default colors.
       config.writeEntry("BackgroundColor", mAppearance.colorList->color(0) );
       config.writeEntry("ForegroundColor", mAppearance.colorList->color(1) );
       config.writeEntry("QuoutedText1",    mAppearance.colorList->color(2) );
       config.writeEntry("QuoutedText2",    mAppearance.colorList->color(3) );
       config.writeEntry("QuoutedText3",    mAppearance.colorList->color(4) );
       config.writeEntry("LinkColor",       mAppearance.colorList->color(5) );
       config.writeEntry("FollowedColor",   mAppearance.colorList->color(6) );
       config.writeEntry("NewMessage",      mAppearance.colorList->color(7) );
       config.writeEntry("UnreadMessage",   mAppearance.colorList->color(8) );
    }
    bool recycleColors = mAppearance.recycleColorCheck->isChecked();
    config.writeEntry("RecycleQuoteColors", recycleColors );

    config.setGroup("Geometry");
    bool longFolderList = mAppearance.longFolderCheck->isChecked();
    config.writeEntry( "longFolderList", longFolderList );

    bool nestedMessages = mAppearance.nestedMessagesCheck->isChecked();
    config.writeEntry( "nestedMessages", nestedMessages );

    config.setGroup("Reader");
    bool htmlMail = mAppearance.htmlMailCheck->isChecked();
    config.writeEntry( "htmlMail", !htmlMail );

    config.setGroup("General");
    bool messageSize = mAppearance.messageSizeCheck->isChecked();
    config.writeEntry( "showMessageSize", messageSize );

  }
  else if( activePage == mComposer.pageIndex )
  {
    config.setGroup("KMMessage");
    config.writeEntry("phrase-reply", 
		      mComposer.phraseReplyEdit->text() );
    config.writeEntry("phrase-reply-all", 
		      mComposer.phraseReplyAllEdit->text() );
    config.writeEntry("phrase-forward", 
		      mComposer.phraseForwardEdit->text() );
    config.writeEntry("indent-prefix", 
		      mComposer.phraseindentPrefixEdit->text() );

    config.setGroup("Composer");
    bool autoSignature = mComposer.autoAppSignFileCheck->isChecked();
    config.writeEntry("signature", autoSignature ? "auto" : "manual" );
    config.writeEntry("smart-quote", mComposer.smartQuoteCheck->isChecked() );
    config.writeEntry("pgp-auto-sign", 
		      mComposer.pgpAutoSignatureCheck->isChecked() );
    config.writeEntry("word-wrap", mComposer.wordWrapCheck->isChecked() );
    config.writeEntry("break-at", mComposer.wrapColumnSpin->value() );
  }  
  else if( activePage == mMime.pageIndex )
  {
    int numValidEntry = 0;
    int numEntry = mMime.tagList->childCount();
    for (int i = 0; i < numEntry; i++) 
    {
      config.setGroup(QString("Mime #%1").arg(i));
      QListViewItem *item = mMime.tagList->firstChild();
      if( item->text(0).length() > 0 )
      {
	config.writeEntry( "name",  item->text(0) );
	config.writeEntry( "value", item->text(1) );
	numValidEntry += 1;
      }
    }
    config.setGroup("General");
    config.writeEntry("mime-header-count", numValidEntry );
  }  
  else if( activePage == mSecurity.pageIndex )
  {
    mSecurity.pgpConfig->applySettings();
  } 
  else if( activePage == mMisc.pageIndex )
  {
    config.setGroup("General");
    config.writeEntry( "empty-trash-on-exit", 
		       mMisc.emptyTrashCheck->isChecked() );
    config.writeEntry( "sendOnCheck",
		       mMisc.sendOutboxCheck->isChecked() );
    config.writeEntry( "send-receipts", 
		       mMisc.sendReceiptCheck->isChecked() );
    config.writeEntry( "compact-all-on-exit", 
		       mMisc.compactOnExitCheck->isChecked() );
    config.writeEntry( "use-external-editor", 
		       mMisc.externalEditorCheck->isChecked() ); 
    config.writeEntry( "external-editor", 
		       mMisc.externalEditorEdit->text() );
    config.writeEntry( "beep-on-mail", 
		       mMisc.beepNewMailCheck->isChecked() );
    config.writeEntry( "msgbox-on-mail", 
		       mMisc.showMessageBoxCheck->isChecked() );
    config.writeEntry( "exec-on-mail", 
		       mMisc.mailCommandCheck->isChecked() );
    config.writeEntry( "exec-on-mail-cmd", 
		       mMisc.mailCommandEdit->text() );
  }

  //
  // Always
  //
  config.setGroup("General");
  config.writeEntry("first-start", false);
  config.sync();

  //
  // Make other components read the new settings
  //
  KMMessage::readConfig();

  QListIterator<KTMainWindow> it(*KTMainWindow::memberList);
  for( it.toFirst(); it.current(); ++it )
  {
    if (it.current()->inherits("KMTopLevelWidget"))
    {
      ((KMTopLevelWidget*)it.current())->readConfig();
    }
  }

}






void ConfigureDialog::saveActiveIdentity( void )
{
  IdentityEntry *entry = mIdentityList.get(mIdentity.mActiveIdentity);
  if( entry != 0 )
  {
    entry->setFullName( mIdentity.nameEdit->text() );
    entry->setOrganization( mIdentity.organizationEdit->text() );
    entry->setEmailAddress( mIdentity.emailEdit->text() );
    entry->setReplyToAddress( mIdentity.replytoEdit->text() );
    entry->setSignatureFileName( mIdentity.signatureFileEdit->text() );
    entry->setSignatureInlineText( mIdentity.signatureTextEdit->text() );
    entry->setSignatureFileIsAProgram(
      mIdentity.signatureExecCheck->isChecked() );
    entry->setUseSignatureFile( mIdentity.signatureFileRadio->isChecked() );
  }
}


void ConfigureDialog::setIdentityInformation( const QString &identity )
{
  if( mIdentity.mActiveIdentity == identity )
  {
    return;
  }

  //
  // 1. Save current settings to the list
  //
  saveActiveIdentity();

  mIdentity.mActiveIdentity = identity;

  //
  // 2. Display the new settings
  //
  bool useSignatureFile;
  IdentityEntry *entry = mIdentityList.get( mIdentity.mActiveIdentity );
  if( entry == 0 )
  {
    mIdentity.nameEdit->setText("");
    mIdentity.organizationEdit->setText("");
    mIdentity.emailEdit->setText("");
    mIdentity.replytoEdit->setText("");
    mIdentity.signatureFileEdit->setText("");
    mIdentity.signatureExecCheck->setChecked( false );
    mIdentity.signatureTextEdit->setText( "" );
    useSignatureFile = true;
  }
  else
  {
    mIdentity.nameEdit->setText( entry->fullName() );
    mIdentity.organizationEdit->setText( entry->organization() );
    mIdentity.emailEdit->setText( entry->emailAddress() );
    mIdentity.replytoEdit->setText( entry->replyToAddress() );
    mIdentity.signatureFileEdit->setText( entry->signatureFileName() );
    mIdentity.signatureExecCheck->setChecked(entry->signatureFileIsAProgram());
    mIdentity.signatureTextEdit->setText( entry->signatureInlineText() );
    useSignatureFile = entry->useSignatureFile();
  }

  if( useSignatureFile == true )
  {
    mIdentity.signatureFileRadio->setChecked(true);
    slotSignatureType(0);
  }
  else
  {
    mIdentity.signatureTextRadio->setChecked(true);
    slotSignatureType(1);
  }
}


QStringList ConfigureDialog::identityStrings( void )
{
  QStringList list;
  for( int i=0; i< mIdentity.identityCombo->count(); i++ )
  {
    list += mIdentity.identityCombo->text(i);
  }
  return( list );
}



void ConfigureDialog::slotNewIdentity( void )
{
  //
  // First. Save current setting to the list. In the dialog box we 
  // can choose to copy from the list so it must be synced.
  //
  saveActiveIdentity();

  //
  // Make and open the dialog 
  //
  NewIdentityDialog *dialog = new NewIdentityDialog( this, "new", true );
  QStringList list = identityStrings();
  dialog->setIdentities( list );

  int result = dialog->exec();
  if( result == QDialog::Accepted )
  {
    QString identityText = dialog->identityText().stripWhiteSpace();
    if( identityText.isEmpty() == false )
    {
      //
      // Add the new identity. Make sure the default identity is 
      // first in the otherwise sorted list
      //
      QString defaultIdentity = list.first();
      list.remove( defaultIdentity );
      list += identityText;
      list.sort();
      list.prepend( defaultIdentity );

      //
      // Set the modifiled list as the valid list in the combo and 
      // make the new identity the current item.
      //
      mIdentity.identityCombo->clear();
      mIdentity.identityCombo->insertStringList(list);
      mIdentity.identityCombo->setCurrentItem( list.findIndex(identityText) );

      if( dialog->duplicateMode() == NewIdentityDialog::ControlCenter )
      {
	mIdentityList.add( identityText, this, true );
      }
      else if( dialog->duplicateMode() == NewIdentityDialog::ExistingEntry )
      {
	mIdentityList.add( identityText, dialog->duplicateText() );
      }
      else
      {
	mIdentityList.add( identityText, this, false );
      }
      slotIdentitySelectorChanged();
    }
  }
  delete dialog;
}


void ConfigureDialog::slotRenameIdentity( void )
{
  RenameIdentityDialog *dialog = new RenameIdentityDialog( this, "new", true );

  QStringList list = identityStrings();
  dialog->setIdentities( mIdentity.identityCombo->currentText(), list );
  
  int result = dialog->exec();
  if( result == QDialog::Accepted )
  {
    int index = mIdentity.identityCombo->currentItem();
    IdentityEntry *entry = mIdentityList.get( index );
    if( entry != 0 )
    { 
      entry->setIdentity( dialog->identityText() );
      mIdentity.mActiveIdentity = entry->identity();      
      mIdentity.identityCombo->clear();
      mIdentity.identityCombo->insertStringList( mIdentityList.identities() );
      mIdentity.identityCombo->setCurrentItem( index );
    }
  }

  delete dialog;
}


void ConfigureDialog::slotRemoveIdentity( void )
{
  int currentItem = mIdentity.identityCombo->currentItem();
  if( currentItem > 0 ) // Item 0 is the default and can not be removed.
  {
    QString msg = i18n(
     "Do you really want to remove the identity\n"
     "named \"%1\" ?").arg(mIdentity.identityCombo->currentText());
    int result = KMessageBox::warningYesNo( this, msg );
    if( result == KMessageBox::Yes )
    {
      mIdentityList.remove( mIdentity.identityCombo->currentText() );
      mIdentity.identityCombo->removeItem( currentItem );
      mIdentity.identityCombo->setCurrentItem( currentItem-1 );
      slotIdentitySelectorChanged();
    }
  }
}


void ConfigureDialog::slotIdentitySelectorChanged( void )
{
  int currentItem = mIdentity.identityCombo->currentItem();
  mIdentity.removeIdentityButton->setEnabled( currentItem != 0 );
  mIdentity.renameIdentityButton->setEnabled( currentItem != 0 );
  setIdentityInformation( mIdentity.identityCombo->currentText() );
}


void ConfigureDialog::slotSignatureType( int id )
{
  bool flag;
  if( id == 0 )
  {
    flag = true;
  }
  else if( id == 1 )
  {
    flag = false;
  }
  else
  {
    return;
  }

  mIdentity.signatureFileLabel->setEnabled( flag );
  mIdentity.signatureFileEdit->setEnabled( flag );
  mIdentity.signatureExecCheck->setEnabled( flag );
  mIdentity.signatureBrowseButton->setEnabled( flag );
  if( flag==true )
  {
    mIdentity.signatureEditButton->setEnabled( 
      !mIdentity.signatureExecCheck->isChecked() );
  }
  else
  {
    mIdentity.signatureEditButton->setEnabled( false );
  }
  mIdentity.signatureTextEdit->setEnabled( !flag );
}


void ConfigureDialog::slotSignatureChooser( void )
{
  KFileDialog *d = new KFileDialog( QDir::homeDirPath(), "*", this, 0, true );
  d->setCaption(i18n("Choose Signature File"));
  
  if( d->exec() == QDialog::Accepted )
  {
    KURL url = d->selectedURL();
    if( url.isEmpty() == true )
    {
      delete d;
      return;
    }

    if( url.isLocalFile() == false )
    {
      KMessageBox::sorry( this, i18n( "Only local files supported yet." ) );
      delete d;
      return;
    }
    
    mIdentity.signatureFileEdit->setText(url.path());
  }
  delete d;
}


void ConfigureDialog::slotSignatureFile( const QString &filename )
{
  QString path = filename.stripWhiteSpace();
  if( mIdentity.signatureFileRadio->isChecked() == true )
  {
    bool state = path.isEmpty() == false ? true : false;
    mIdentity.signatureEditButton->setEnabled( state );
    mIdentity.signatureExecCheck->setEnabled( state );
  }
}


void ConfigureDialog::slotSignatureEdit( void )
{
  QString fileName = mIdentity.signatureFileEdit->text().stripWhiteSpace();
  if( fileName.isEmpty() == true )
  {
    KMessageBox::error( this, i18n("You must specify a filename") );
    return;
  }
  
  QFileInfo fileInfo( fileName );
  if( fileInfo.isDir() == true )
  {
    QString msg = i18n("You have specified a directory\n\n%1").arg(fileName);
    KMessageBox::error( this, msg );
    return;
  }

  if( fileInfo.exists() == false )
  {
    // Create the file first
    QFile file( fileName );
    if( file.open( IO_ReadWrite ) == false )
    {
      QString msg = i18n("Unable to create new file at\n\n%1").arg(fileName);
      KMessageBox::error( this, msg );
      return;
    }
  }

  QString cmdline( "kedit %f" );
  if( cmdline.length() == 0 )
  {
    cmdline = DEFAULT_EDITOR_STR;
  }

  QString argument = "\"" + fileName + "\"";
  ApplicationLaunch kl(cmdline.replace(QRegExp("\\%f"), argument ));
  kl.run();
}


void ConfigureDialog::slotSignatureExecMode( bool state )
{
  mIdentity.signatureEditButton->setEnabled( !state );
}

//
// Network page
//

void ConfigureDialog::slotSendmailChooser( void )
{
  KFileDialog dialog("/", "*", this, 0, true );
  dialog.setCaption(i18n("Choose Sendmail Location") );

  if( dialog.exec() == QDialog::Accepted )
  {
    KURL url = dialog.selectedURL();
    if( url.isEmpty() == true )
    {
      return;
    }

    if( url.isLocalFile() == false )
    {
      KMessageBox::sorry( 0L, i18n( "Only local files allowed." ) );
      return;
    }
    
    mNetwork.sendmailLocationEdit->setText( url.path() );
  }
}


void ConfigureDialog::slotSendmailType( int id )
{
  bool useSendmail;
  if( id == 0 )
  {
    useSendmail = true;
  }
  else if( id == 1 )
  {
    useSendmail = false;
  }
  else
  {
    return;
  }

  mNetwork.sendmailLocationEdit->setEnabled( useSendmail );
  mNetwork.sendmailChooseButton->setEnabled( useSendmail );
  mNetwork.smtpServerEdit->setEnabled( !useSendmail );
  mNetwork.smtpPortEdit->setEnabled( !useSendmail );
}




void ConfigureDialog::slotAccountSelected( void )
{
  mNetwork.modifyAccountButton->setEnabled( true );
  mNetwork.removeAccountButton->setEnabled( true );
}



void ConfigureDialog::slotAddAccount( void )
{
  KMAcctSelDlg accountSelectorDialog( this, i18n("Select Account") );
  if( accountSelectorDialog.exec() != QDialog::Accepted )
  {
    return;
  }

  const char *accountType = 0;
  switch( accountSelectorDialog.selected() )
  {
    case 0:
      accountType = "local";
    break;

    case 1:
      accountType = "pop";
    break;

    case 2:
      accountType = "experimental pop";
    break;

    default:
      KMessageBox::sorry( this, i18n("Unknown account type selected") );
      return;
    break;
  }

  KMAccount *account = kernel->acctMgr()->create(accountType,i18n("Unnamed"));
  if( account == 0 )
  {
    KMessageBox::sorry( this, i18n("Unable to create account") );
    return;
  }

  account->init(); // fill the account fields with good default values

  AccountDialog *dialog = new AccountDialog( account, identityStrings(), this);
  dialog->setCaption( i18n("Add account") );
  if( dialog->exec() == QDialog::Accepted )
  {
    QListViewItem *listItem = 
      new QListViewItem(mNetwork.accountList,account->name(),account->type());
    if( account->folder() ) 
    {
      listItem->setText( 2, account->folder()->name() );
    }
  }
  else
  {
    kernel->acctMgr()->remove(account);
  }
  delete dialog;
}



void ConfigureDialog::slotModifySelectedAccount( void )
{
  QListViewItem *listItem = mNetwork.accountList->selectedItem(); 
  if( listItem == 0 )
  {
    return;
  }

  KMAccount *account = kernel->acctMgr()->find( listItem->text(0) );
  if( account == 0 )
  {
    KMessageBox::sorry( this, i18n("Unable to locate account") );
    return;
  }

  AccountDialog *dialog = new AccountDialog( account, identityStrings(), this);
  dialog->setCaption( i18n("Modify account") );
  if( dialog->exec() == QDialog::Accepted )
  {
    listItem->setText( 0, account->name() );
    listItem->setText( 1, account->type() );
    if( account->folder() ) 
    { 
      listItem->setText( 2, account->folder()->name() );
    }
  }
  delete dialog;
}



void ConfigureDialog::slotRemoveSelectedAccount( void )
{
  QListViewItem *listItem = mNetwork.accountList->selectedItem(); 
  if( listItem == 0 )
  {
    return;
  }
  
  KMAccount *account = kernel->acctMgr()->find( listItem->text(0) );
  if( account == 0 )
  {
    KMessageBox::sorry( this, i18n("Unable to locate account") );
    return;
  }

  if( !kernel->acctMgr()->remove(account) )
  {
    return;
  }

  mNetwork.accountList->takeItem( listItem );
  if( mNetwork.accountList->childCount() == 0 )
  {
    mNetwork.modifyAccountButton->setEnabled( false );
    mNetwork.removeAccountButton->setEnabled( false );
  }
  else
  {
    mNetwork.accountList->setSelected(mNetwork.accountList->firstChild(),true);
  }
}


void ConfigureDialog::slotCustomFontSelectionChanged( void )
{
  bool flag = mAppearance.customFontCheck->isChecked();
  mAppearance.fontLocationLabel->setEnabled( flag );
  mAppearance.fontLocationCombo->setEnabled( flag );
  mAppearance.fontChooser->setEnabled( flag );
}

void ConfigureDialog::slotFontSelectorChanged( int index )
{
  if( index < 0 || index >= mAppearance.fontLocationCombo->count() )
  {
    return; // Should never happen, but it is better to check.
  }

  //
  // Save current fontselector setting before we install the new
  //
  if( mAppearance.activeFontIndex >= 0 )
  {
    mAppearance.fontString[mAppearance.activeFontIndex] = 
      kfontToStr( mAppearance.fontChooser->font() );
  }
  mAppearance.activeFontIndex = index;

  //
  // Display the new setting
  //
  if( mAppearance.fontString[index].isEmpty() == false ) 
    mAppearance.fontChooser->setFont(kstrToFont(mAppearance.fontString[index]));
  
  //
  // Disable Family and Size list if we have selected a qoute font 
  //
  bool enable = index != 3 && index != 4 && index != 5;
  mAppearance.fontChooser->enableColumn( 
    KFontChooser::FamilyList|KFontChooser::SizeList, enable );
}

void ConfigureDialog::slotCustomColorSelectionChanged( void )
{
  bool state = mAppearance.customColorCheck->isChecked();
  mAppearance.colorList->setEnabled( state );
  if (state && (mAppearance.colorList->currentItem() < 0))
     mAppearance.colorList->setCurrentItem(0);
  mAppearance.recycleColorCheck->setEnabled( state );
}

void ConfigureDialog::slotWordWrapSelectionChanged( void )
{
  mComposer.wrapColumnSpin->setEnabled(mComposer.wordWrapCheck->isChecked());
}


void ConfigureDialog::slotMimeHeaderSelectionChanged( void )
{
  mMime.currentTagItem = mMime.tagList->selectedItem();
  if( mMime.currentTagItem != 0 )
  {
    mMime.tagNameEdit->setText( mMime.currentTagItem->text(0) );
    mMime.tagValueEdit->setText( mMime.currentTagItem->text(1) );
    mMime.tagNameEdit->setEnabled(true);
    mMime.tagValueEdit->setEnabled(true);
    mMime.tagNameLabel->setEnabled(true);
    mMime.tagValueLabel->setEnabled(true);
  }
}


void ConfigureDialog::slotMimeHeaderNameChanged( const QString &text )
{
  if( mMime.currentTagItem != 0 )
  {
    mMime.currentTagItem->setText(0, text );
  }
}


void ConfigureDialog::slotMimeHeaderValueChanged( const QString &text )
{
  if( mMime.currentTagItem != 0 )
  {
    mMime.currentTagItem->setText(1, text );
  }
}


void ConfigureDialog::slotNewMimeHeader( void )
{
  QListViewItem *listItem = new QListViewItem( mMime.tagList, "", "" );
  mMime.tagList->setCurrentItem( listItem );
  mMime.tagList->setSelected( listItem, true );

  mMime.currentTagItem = mMime.tagList->selectedItem();
  if( mMime.currentTagItem != 0 )
  {
    mMime.tagNameEdit->setEnabled(true);
    mMime.tagValueEdit->setEnabled(true);
    mMime.tagNameLabel->setEnabled(true);
    mMime.tagValueLabel->setEnabled(true);
    mMime.tagNameEdit->setFocus();
  }
}


void ConfigureDialog::slotDeleteMimeHeader( void )
{
  if( mMime.currentTagItem != 0 )
  {
    QListViewItem *next = mMime.currentTagItem->itemAbove();
    if( next == 0 )
    {
      next = mMime.currentTagItem->itemBelow();
    }

    mMime.tagNameEdit->clear();
    mMime.tagValueEdit->clear();
    mMime.tagNameEdit->setEnabled(false);
    mMime.tagValueEdit->setEnabled(false);
    mMime.tagNameLabel->setEnabled(false);
    mMime.tagValueLabel->setEnabled(false);

    mMime.tagList->takeItem( mMime.currentTagItem );
    mMime.currentTagItem = 0;

    if( next != 0 )
    {
      mMime.tagList->setSelected( next, true );
    }
  }
}


void ConfigureDialog::slotExternalEditorSelectionChanged( void )
{
  bool flag = mMisc.externalEditorCheck->isChecked();
  mMisc.externalEditorEdit->setEnabled( flag );
  mMisc.externalEditorChooseButton->setEnabled( flag );
  mMisc.externalEditorLabel->setEnabled( flag );
  mMisc.externalEditorHelp->setEnabled( flag );
}


void ConfigureDialog::slotMailCommandSelectionChanged( void )
{
  bool flag = mMisc.mailCommandCheck->isChecked();
  mMisc.mailCommandEdit->setEnabled( flag );
  mMisc.mailCommandChooseButton->setEnabled( flag );
  mMisc.mailCommandLabel->setEnabled( flag );
}


void ConfigureDialog::slotExternalEditorChooser( void )
{
  KFileDialog dialog("/", "*", this, 0, true );
  dialog.setCaption(i18n("Choose External Editor") );

  if( dialog.exec() == QDialog::Accepted )
  {
    KURL url = dialog.selectedURL();
    if( url.isEmpty() == true )
    {
      return;
    }

    if( url.isLocalFile() == false )
    {
      KMessageBox::sorry( 0L, i18n( "Only local files allowed." ) );
      return;
    }
    
    mMisc.externalEditorEdit->setText( url.path() );
  }
}


void ConfigureDialog::slotMailCommandChooser( void )
{
  KFileDialog dialog("/", "*", this, 0, true );
  dialog.setCaption(i18n("Choose External Command") );

  if( dialog.exec() == QDialog::Accepted )
  {
    KURL url = dialog.selectedURL();
    if( url.isEmpty() == true )
    {
      return;
    }

    if( url.isLocalFile() == false )
    {
      KMessageBox::sorry( 0L, i18n( "Only local files allowed." ) );
      return;
    }
    
    mMisc.mailCommandEdit->setText( url.path() );
  }
}


IdentityEntry::IdentityEntry( void )
{
  mSignatureFileIsAProgram = false;
  mUseSignatureFile = true;
}


QString IdentityEntry::identity() const
{
  return( mIdentity );
}

QString IdentityEntry::fullName() const
{
  return( mFullName );
}

QString IdentityEntry::organization() const 
{
  return( mOrganization );
}

QString IdentityEntry::emailAddress() const
{
  return( mEmailAddress );
}

QString IdentityEntry::replyToAddress() const
{
  return( mReplytoAddress );
}

QString IdentityEntry::signatureFileName( bool exportIdentity ) const 
{
  if( exportIdentity == true && mSignatureFileIsAProgram == true )
  {
    printf("exportIdentity=%d\n", exportIdentity );
    printf("mSignatureFileIsAProgram=%d\n", mSignatureFileIsAProgram );

    return( mSignatureFileName + "|" );
  }
  else
  {
    return( mSignatureFileName );
  }
}

QString IdentityEntry::signatureInlineText() const
{
  return( mSignatureInlineText );
}

bool IdentityEntry::signatureFileIsAProgram() const
{
  return( mSignatureFileIsAProgram );
}

bool IdentityEntry::useSignatureFile() const
{
  return( mUseSignatureFile );
}


void IdentityEntry::setIdentity( const QString &identity )
{
  mIdentity = identity;
}

void IdentityEntry::setFullName( const QString &fullName )
{
  mFullName = fullName;
}

void IdentityEntry::setOrganization( const QString &organization )
{
  mOrganization = organization;
}

void IdentityEntry::setEmailAddress( const QString &emailAddress )
{
  mEmailAddress = emailAddress;
}

void IdentityEntry::setReplyToAddress( const QString &replytoAddress )
{
  mReplytoAddress = replytoAddress;
}

void IdentityEntry::setSignatureFileName( const QString &signatureFileName,
					  bool importIdentity )
{
  if( importIdentity == true )
  {
    if( signatureFileName.right(1) == "|" )
    {
      mSignatureFileName=signatureFileName.left(signatureFileName.length()-1);
      setSignatureFileIsAProgram(true);
    }
    else
    {
      mSignatureFileName=signatureFileName;
      setSignatureFileIsAProgram(false);
    }
  }
  else
  {
    mSignatureFileName=signatureFileName;
  }
}

void IdentityEntry::setSignatureInlineText( const QString &signatureInlineText)
{
  mSignatureInlineText = signatureInlineText;
}

void IdentityEntry::setSignatureFileIsAProgram( bool signatureFileIsAProgram )
{
  mSignatureFileIsAProgram = signatureFileIsAProgram;
}

void IdentityEntry::setUseSignatureFile( bool useSignatureFile )
{
  mUseSignatureFile = useSignatureFile;
}





IdentityList::IdentityList()
{
  mList.setAutoDelete(true);
}


QStringList IdentityList::identities()
{
  QStringList list;
  for( IdentityEntry *e = mList.first(); e != 0; e = mList.next() )
  {
    list += e->identity();
  }
  return( list );
}


IdentityEntry *IdentityList::get( const QString &identity )
{
  for( IdentityEntry *e = mList.first(); e != 0; e = mList.next() )
  {
    if( identity == e->identity() )
    {
      return( e );
    }
  }
  return( 0 );
}


IdentityEntry *IdentityList::get( uint index )
{
  return( mList.at(index) );
}


void IdentityList::remove( const QString &identity )
{
  IdentityEntry *e = get(identity);
  if( e != 0 )
  {
    mList.remove(e);
  }
}



void IdentityList::importData()
{
  IdentityEntry entry;
  QStringList identities = KMIdentity::identities();
  QStringList::Iterator it;
  for( it = identities.begin(); it != identities.end(); ++it ) 
  {
    KMIdentity ident( *it );
    ident.readConfig();
    entry.setIdentity( ident.identity() );
    entry.setFullName( ident.fullName() );
    entry.setOrganization( ident.organization() );
    entry.setEmailAddress( ident.emailAddr() );
    entry.setReplyToAddress( ident.replyToAddr() );
    entry.setSignatureFileName( ident.signatureFile(), true );
    entry.setSignatureInlineText( ident.signatureInlineText() );
    entry.setUseSignatureFile( ident.useSignatureFile() );
    add( entry );    
  } 
}


void IdentityList::exportData()
{
  QStringList ids;
  for( IdentityEntry *e = mList.first(); e != 0; e = mList.next() ) 
  {
    KMIdentity ident( e->identity() );
    ident.setFullName( e->fullName() );
    ident.setOrganization( e->organization() );
    ident.setEmailAddr( e->emailAddress() );
    ident.setReplyToAddr( e->replyToAddress() );
    ident.setUseSignatureFile( e->useSignatureFile() );
    ident.setSignatureFile( e->signatureFileName(true) );
    ident.setSignatureInlineText( e->signatureInlineText() );
    ident.writeConfig();
    ids.append( e->identity() );
  }

  KMIdentity::saveIdentities( ids, true );
}




void IdentityList::add( const IdentityEntry &entry )
{
  if( get( entry.identity() ) != 0 )
  {
    return; // We can not have duplicates.
  }

  mList.append( new IdentityEntry(entry) );
}


void IdentityList::add( const QString &identity, const QString &copyFrom )
{
  if( get( identity ) != 0 )
  {
    return; // We can not have duplicates.
  }

  IdentityEntry newEntry;

  IdentityEntry *src = get( copyFrom );
  if( src != 0 )
  {
    newEntry = *src;
  }

  newEntry.setIdentity( identity );
  add( newEntry );
}


void IdentityList::add( const QString &identity, QWidget *parent, 
			bool useControlCenter )
{
  if( get( identity ) != 0 )
  {
    return; // We can not have duplicates.
  }

  IdentityEntry newEntry;

  newEntry.setIdentity( identity );
  if( useControlCenter == true )
  {
    //
    // The returned filename is empty if the file exists but 
    // is not readable so we only have to test if the file exists.
    //
    QString configFileName = locate( "config", "emaildefaults" );
    QFileInfo fileInfo(configFileName);
    if( fileInfo.exists() == false )
    {
      QString msg = i18n(""
        "The email configuration file could not be located.\n"
	"You can create one in Control Center.\n\n"
	"(Search for \"email\" in Control Center)");
      KMessageBox::error( parent, msg );
    }
    else
    {
      KSimpleConfig config( configFileName, false );
      config.setGroup("UserInfo");

      newEntry.setFullName( config.readEntry( "FullName", "" ) );
      newEntry.setEmailAddress( config.readEntry( "EmailAddress", "" ) );
      newEntry.setOrganization( config.readEntry( "Organization", "" ) );
      newEntry.setReplyToAddress( config.readEntry( "ReplyAddr", "" ) );
    }
  }
  add( newEntry );
}








void IdentityList::update( const IdentityEntry &entry )
{
  for( IdentityEntry *e = mList.first(); e != 0; e = mList.next() )
  {
    if( entry.identity() == e->identity() )
    {
      *e = entry;
      return;
    }
  }
}







