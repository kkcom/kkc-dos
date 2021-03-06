#include <dos.h>
#include <direct.h>
#include <io.h>
#include <stdlib.h>
#include <conio.h>
#include <mem.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "hard.h"
#include "idf.h"
#include "help.h"

#define GAUCHE 0x4B
#define DROITE 0x4D

extern struct key K[nbrkey];

// Pour Statistique;
int St_App;
int St_Dir;

struct player {
    char *Filename;
    char *Meneur;
    char *Titre;
    unsigned long Checksum;
    short ext;      // Numero d'extension
    short pres;     // 0 si pas trouv� sinon numero du directory
    char type;
    } *app[5000];

char dir[50][128]; // 50 directory diff�rents de 128 caracteres

short nbr;    // nombre d'application lu dans les fichiers KKR
short nbrdir; //

char OldY;

void Search(char *nom);
void ApplSearch(void);
void ClearAllSpace(char *name);

int posy;

/*-------------------------*
 * Procedure en Assembleur *
 *-------------------------*/

char GetDriveReady(char i);
#pragma aux GetDriveReady = \
	"mov ah,19h" \
	"int 21h" \
	"mov ch,al" \
	"mov ah,0Eh" \
	"int 21h" \
	"mov ah,19h" \
	"int 21h" \
	"sub al,dl" \
	"mov cl,al" \
	"mov dl,ch" \
	"mov ah,0Eh" \
	"int 21h" \
	parm [dl] \
    value [cl];

int sort_function(const void *a,const void *b)
{
struct key *a1,*b1;

a1=(struct key*)a;
b1=(struct key*)b;

// return (a1->numero)-(b1->numero);

if (a1->type!=b1->type) return (a1->type)-(b1->type);

return strcmp(a1->ext,b1->ext);		// ou format ?
}




void IdfListe(void)
{
int y;

int n;
int info;

SaveEcran();

info=0;

qsort((void*)K,nbrkey,sizeof(struct key),sort_function);

y=3;

PrintAt(2,1,"%-77s","List of the format");

for (n=0;n<nbrkey;n++)  {
    if ( (n==0) | (K[n-1].type!=K[n].type) )
        {
        ChrLin(1,y,78,196);
        switch(K[n+1].type)
            {
            case 1: PrintAt(4,y,"Module");
                break;
            case 2: PrintAt(4,y,"Sample");
                break;
            case 3: PrintAt(4,y,"Archive");
                break;
            case 4: PrintAt(4,y,"Bitmap");
                break;
            case 5: PrintAt(4,y,"Animation");
                break;
            case 6: PrintAt(4,y,"Others");
                break;
            }
        y++;
        if ( (y==48) | (n==nbrkey-1) )
            {
            y=3;
            Wait(0,0,0);
            ChrWin(1,3,78,48,32);
            ColWin(1,3,78,48,10*16+1);
            }
        }

    if (n&1==1)
        {
        ColLin(2,y,3,  10*16+3);
        ColLin(5,y,1,  10*16+3);
        ColLin(6,y,32, 10*16+4);
        ColLin(38,y,6, 10*16+3);
        ColLin(44,y,29,10*16+5);
        ColLin(73,y,1, 10*16+3);
        ColLin(74,y,4, 10*16+3);
        }
        else
        {
        ColLin(2,y,3,  15*16+3);
        ColLin(5,y,1,  15*16+3);
        ColLin(6,y,32, 15*16+4);
        ColLin(38,y,6, 15*16+3);
        ColLin(44,y,29,15*16+5);
        ColLin(73,y,1, 15*16+3);
        ColLin(74,y,4, 15*16+3);
        }

    PrintAt(2,y,"%3s %-32s from %29s %4s",K[n].ext,K[n].format,K[n].pro,K[n].other==1 ? "Info" : "----");

    if (K[n].other==1) info++;

    y++;
    if ( (y==48) | (n==nbrkey-1) )
        {
        y=3;
        Wait(0,0,0);
        ChrWin(1,3,78,48,32);
        ColWin(1,3,78,48,10*16+1);
        }
    }

// printf("\n%3d formats",nbrkey);
// printf("\n%3d informations\n\n",info);

ChargeEcran();
}

void SaveCfg(void)
{
int m,n,t;
FILE *fic;
short taille;
char chaine[256];
ENTIER e;

Cfg->FenTyp[0]=0;
Cfg->FenTyp[1]=0;

fic=fopen(Fics->CfgFile,"wb");
fwrite((void*)Cfg,sizeof(struct config),1,fic);

for(n=0;n<16;n++)
    {
    fwrite(&(Mask[n]->Ignore_Case),1,1,fic);
    fwrite(&(Mask[n]->Other_Col),1,1,fic);
    taille=strlen(Mask[n]->chaine);
    fwrite(&taille,2,1,fic);
    fwrite(Mask[n]->chaine,taille,1,fic);
    taille=strlen(Mask[n]->title);
    fwrite(&taille,2,1,fic);
    fwrite(Mask[n]->title,taille,1,fic);
    }


for(t=0;t<2;t++)
{

strcpy(chaine,Fics->LastDir);
fwrite(chaine,256,1,fic);           // Path
e=1;
fwrite(&e,sizeof(ENTIER),1,fic);    // Order

e=0;
fwrite(&e,sizeof(ENTIER),1,fic);    // Sorting

n=0;
fwrite(&n,4,1,fic);                 // Nombre de selectionne

strcpy(chaine,"KK.EXE");
m=strlen(chaine);
fwrite(&m,4,1,fic);                 // Fichier sur lequel se trouve le curseur
fwrite(chaine,1,m,fic);

fwrite(&e,sizeof(ENTIER),1,fic);    // Position du curseur
}

fclose(fic);
}


// Retourne -1 en cas d'erreur
//           0 si tout va bien
int LoadCfg(void)
{
int n;
FILE *fic;
short taille;


fic=fopen(Fics->CfgFile,"rb");
if (fic==NULL) return -1;

fread((void*)Cfg,sizeof(struct config),1,fic);


for(n=0;n<16;n++)
    {
    fread(&(Mask[n]->Ignore_Case),1,1,fic);
    fread(&(Mask[n]->Other_Col),1,1,fic);
    fread(&taille,2,1,fic);
    fread(Mask[n]->chaine,taille,1,fic);
    Mask[n]->chaine[taille]=0;
    fread(&taille,2,1,fic);
    fread(Mask[n]->title,taille,1,fic);
    Mask[n]->title[taille]=0;
    }


fclose(fic);

return 0;
}




//---------------------------------
// Change drive of current window -
//---------------------------------
void ListDrive(char *lstdrv)
{
char drive[26];
short m,n,x,l,nbr;
signed char i;

int car;


nbr=0;

for (i=0;i<26;i++)
    {
    drive[i]=GetDriveReady(i);
    if (drive[i]==0)
        nbr++;
    }

l=76/nbr;
if (l>3) l=3;
if (nbr<5) l=6;
if (nbr<2) l=9;
x=(80-(l*nbr))/2;

SaveEcran();

WinCadre(x-2,6,x+l*nbr+1,11,0);
ColWin(x-1,7,x+l*nbr,10,0*16+1);
ChrWin(x-1,7,x+l*nbr,10,32);

WinCadre(x-1,8,x+l*nbr,10,1);

PrintAt(x,7,"Select the drive");

m=x+l/2;
for (n=0;n<26;n++)
    {
    if (drive[n]==0)
        {
        drive[n]=m;
        AffChr(m,9,n+'A');
        if (lstdrv[n]==0)
            AffCol(drive[n],9,0*16+1);
            else
            AffCol(drive[n],9,2*16+1);
        m+=l;
        }
        else
        {
        drive[n]=0;
        lstdrv[n]=0;
        }
    }


i=-1;
car=DROITE*256;

do	{
    do {
        if ( (LO(car)==32) & (drive[i]!=0) )
            {
            if (lstdrv[i]==0) lstdrv[i]=1; else lstdrv[i]=0;
            car=9;
            }

        if (lstdrv[i]==0)
            AffCol(drive[i],9,0*16+1);
            else
            AffCol(drive[i],9,2*16+1);

        if (HI(car)==GAUCHE) i--;
        if (HI(car)==DROITE) i++;
        if (HI(car)==0xF) i--;
        if (LO(car)==9) i++;

        if (i==26) i=0;
        if (i<0) i=25;
        } while (drive[i]==0);

/*    if (HI(car)==0)
        {
        car=(toupper(car)-'A');
        if ( (car>=0) & (car<26) )
            if (drive[car]!=0)
                {
                i=car;
                car=13;
                break;
                }
       } */

    

    if (lstdrv[i]==0)
        AffCol(drive[i],9,1*16+5);
        else
        AffCol(drive[i],9,2*16+5);

    car=Wait(0,0,0);

    if (lstdrv[i]==0)
        AffCol(drive[i],9,0*16+1);
        else
        AffCol(drive[i],9,2*16+1);

} while ( (LO(car)!=27) & (LO(car)!=13));

ChargeEcran();

if (car==27)
    for (n=0;n<26;n++)
        lstdrv[n]=0;

}


void main(short argc,char **argv)
{
FILE *fic;
char chaine[256];
short n;
int car;

char *path;



OldY=(*(char*)(0x484))+1;

path=GetMem(256);

strcpy(path,*argv);
for (n=strlen(path);n>0;n--) {
    if (path[n]=='\\') {
        path[n]=0;
        break;
        }
    }


Cfg=GetMem(sizeof(struct config));
Fics=GetMem(sizeof(struct fichier));

Mask=GetMem(sizeof(struct PourMask*)*16);
for (n=0;n<16;n++)
    Mask[n]=GetMem(sizeof(struct PourMask));

Fics->FicIdfFile=GetMem(256);
strcpy(Fics->FicIdfFile,path);
strcat(Fics->FicIdfFile,"\\idfext.rb");

Fics->CfgFile=GetMem(256);
strcpy(Fics->CfgFile,path);
strcat(Fics->CfgFile,"\\kkrb.cfg");

Fics->view=GetMem(256);
strcpy(Fics->view,path);
strcat(Fics->view,"\\view");

Fics->edit=GetMem(256);
strcpy(Fics->edit,path);
strcat(Fics->edit,"\\edit");

Fics->path=GetMem(256);
strcpy(Fics->path,path);

Fics->help=GetMem(256);
strcpy(Fics->help,path);
strcat(Fics->help,"\\kk_setup.hlp");

Fics->LastDir=GetMem(256);
strcpy(Fics->LastDir,path);

if (LoadCfg()==-1)
    {
    char defcol[48]={43,37,30, 31,22,17,  0, 0, 0, 58,58,50,
                     44,63,63, 63,63,21, 43,37,30,  0, 0, 0,
                     63,63, 0, 63,63,63, 43,37,30, 63, 0, 0,
                      0,63, 0,  0, 0,63,  0, 0, 0,  0, 0, 0};

    memcpy(Cfg->palette,defcol,48);
    Cfg->KeyAfterShell=0;
    Cfg->wmask=0;

    Cfg->TailleY=50;
    Cfg->SaveSpeed=7200;
    Cfg->fentype=4;

    strcpy(Mask[0]->title,"C Style");
    strcpy(Mask[0]->chaine,"asm break case cdecl char const continue default do double else enum extern far float for goto huge if int interrupt long near pascal register short signed sizeof static struct switch typedef union unsigned void volatile while @");
    Mask[0]->Ignore_Case=0;
    Mask[0]->Other_Col=1;

    strcpy(Mask[1]->title,"Pascal Style");
    strcpy(Mask[1]->chaine,"absolute and array begin case const div do downto else end external file for forward function goto if implementation in inline interface interrupt label mod nil not of or packed procedure program record repeat ");
    strcat(Mask[1]->chaine,"set shl shr string then to type unit until uses var while with xor @");
    Mask[1]->Ignore_Case=1;
    Mask[1]->Other_Col=1;

    strcpy(Mask[2]->title,"Assembler Style");
    strcpy(Mask[2]->chaine,"aaa aad aam aas adc add and arpl bound bsf bsr bswap bt btc btr bts call cbw cdq clc cld cli clts cmc cmp cmps cmpxchg cwd cwde ");
    strcat(Mask[2]->chaine,"daa das dec div enter esc hlt idiv imul in inc ins int into invd invlpg iret iretd jcxz jecxz jmp ");
    strcat(Mask[2]->chaine,"ja jae jb jbe jc jcxz je jg jge jl jle jna jnae jnb jnbe jnc jne jng jnge jnl jnle jno jnp jns jnz jo jp jpe jpo js jz ");
    strcat(Mask[2]->chaine,"lahf lar lds lea leave les lfs lgdt lidt lgs lldt lmsw lock lodsw lodsb lodsd loop loope loopz loopnz loopne lsl lss ");
    strcat(Mask[2]->chaine,"ltr mov movs movsx movsz mul neg nop not or out outs pop popa popad push pusha pushad pushf pushfd ");
    strcat(Mask[2]->chaine,"rcl rcr rep repe repz repne repnz ret retf rol ror sahf sal shl sar sbb scas ");
    strcat(Mask[2]->chaine,"setae setnb setb setnae setbe setna sete setz setne setnz setl setng setge setnl setle setng setg setnle ");
    strcat(Mask[2]->chaine,"sets setns setc setnc seto setno setp setpe setnp setpo sgdt ");
    strcat(Mask[2]->chaine,"sidt shl shr shld shrd sldt smsw stc std sti stos str sub test verr verw wait fwait wbinvd xchg xlat xlatb xor @");
    strcat(Mask[2]->chaine,"db dw dd endp ends assume");
    Mask[1]->Ignore_Case=1;
    Mask[1]->Other_Col=1;

    strcpy(Mask[15]->title,"User Defined Style");
    strcpy(Mask[15]->chaine,"ketchup killers redbug access darkangel katana cray magic fred cobra @");
    Mask[15]->Ignore_Case=1;
    Mask[15]->Other_Col=1;
    }

TXTMode(50);
NoFlash();

Font8x8();

SetPal(0, 43, 37, 30);
SetPal(1, 31, 22, 17);
SetPal(2, 0, 0, 0);
SetPal(3, 58, 58, 50);
SetPal(4, 44, 63, 63);
SetPal(5, 63, 63, 21);
SetPal(6,43,37,30);
SetPal(7,  0,  0,  0);

SetPal(10, 43, 37, 30);

SetPal(15, 47, 41, 34);

WinCadre(0,0,79,49,1);
ChrWin(1,1,78,48,32);
ColWin(1,1,78,48,10*16+1);
ColLin(1,1,78,10*16+5);
WinLine(1,2,78,0);

PrintAt(21,1,"Setup of Ketchup Killers Commander");


posy=3;

strcpy(chaine,"c:\\dos\\kk.bat");
fic=fopen(chaine,"wt");

fprintf(fic,"@%s\\kk.exe\n",path);
fprintf(fic,"@REM This file was making by KK_SETUP\n");

fclose(fic);

strcpy(chaine,"c:\\dos\\kk_desc.bat");
fic=fopen(chaine,"wt");

fprintf(fic,"@%s\\kk_desc.exe %%1 \n",path);
fprintf(fic,"@REM This file was making by KK_SETUP\n");

fclose(fic);

strcat(chaine,"\\trash");
mkdir(chaine);


PrintAt(10, 5,"F1: Help");

PrintAt(10, 9,"F2: Search Application");

PrintAt(10,13,"F3: Load KK_SETUP.INI");

PrintAt(10,17,"F4: List all the format");

do
{

car=Wait(0,0,0);

switch(HI(car))
    {
    case 0x3B:  // F1
        SaveEcran();
        ChrWin(0,0,79,49,32);
        Help();
        ChargeEcran();
        break;
    case 0x3C:  // F2
        ApplSearch();
        break;
    case 0x3D:  // F3
        ConfigFile();
        break;
    case 0x3E:  // F4
        IdfListe();
        break;
    }

} while ( (HI(car)!=0x44) & (LO(car)!=27) );

PrintAt(10,35,"KK.BAT is now in PATH -> You could run KK from everywhere");
PrintAt(10,37,"KK_DESC.BAT is now in path -> IDEM");
PrintAt(10,36,"%s is done",chaine);

PrintAt(29,48,"Press a key to continue");
ColLin(1,48,78,0*16+2);

Wait(0,0,0);
TXTMode(OldY);

SaveCfg();
}


void ApplSearch(void)
{
char lstdrv[26];
short n,t;
char ch[256];
FILE *fic;

SaveEcran();

nbr=0;

for (n=2;n<26;n++)
    lstdrv[n]=1;
lstdrv[0]=0;
lstdrv[1]=0;

ListDrive(lstdrv);


for (n=0;n<26;n++)
    {
    sprintf(ch,"%c:\\",n+'A');
    if (lstdrv[n]==1)
        KKR_Search(ch);
    }

nbrdir=0;

for (n=0;n<26;n++)
    {
    sprintf(ch,"%c:\\*.*",n+'A');
    if (lstdrv[n]==1)
        Search(ch);
    }

fic=fopen("idfext.rb","wb");
if (fic!=NULL) {
	fwrite("RedBLEXU",1,8,fic);

    fputc(_getdrive()-1,fic);

	t=0;

	for(n=0;n<nbr;n++)
        if (app[n]->pres!=0) t++;

	fwrite(&t,1,2,fic);

	for(n=0;n<nbr;n++)
        if (app[n]->pres!=0)  {
            char sn;
            char *a;

            a=app[n]->Filename;
            sn=strlen(a);
            fwrite(&sn,1,1,fic);
            fwrite(a,sn,1,fic);

            a=app[n]->Titre;
            sn=strlen(a);
            fwrite(&sn,1,1,fic);
            fwrite(a,sn,1,fic);

            a=app[n]->Meneur;
            sn=strlen(a);
            fwrite(&sn,1,1,fic);
            fwrite(a,sn,1,fic);

            fwrite(&(app[n]->ext),2,1,fic);    // Numero de format
            fwrite(&(app[n]->pres),2,1,fic);   // Numero directory

            fwrite(&(app[n]->type),1,1,fic);   // Numero directory
            }

	fwrite(&nbrdir,1,2,fic);

	for(n=0;n<nbrdir;n++)
		fwrite(dir[n],1,128,fic);

	fclose(fic);
	}

PrintAt(29,48,"Press a key to continue");
ColLin(1,48,78,0*16+2);

Wait(0,0,0);
ColWin(1,1,78,48,0*16+1);
ChrWin(1,1,78,48,32);  // '�'

PrintAt(10,10,"Stat.");
PrintAt(10,12,"I have founded %3d applications",St_App);
PrintAt(10,13,"            in %3d directories",St_Dir);

Wait(0,0,0);

ChargeEcran();
}

char KKR_Read(FILE *Fic)
{
char Key[4];
char KKType;
char Comment[255],SComment;
char Titre[255],STitre;
char Meneur[255],SMeneur;
char Filename[255],SFilename;
int Checksum;
short format;

char Code;
char fin;

fin=0;

fread(Key,4,1,Fic);
if (!strncmp(Key,"KKRB",4))
    do {
    fread(&Code,1,1,Fic);
    switch(Code)  {
        case 0:             // Commentaire (sans importance)
            fread(&SComment,1,1,Fic);
            fread(Comment,SComment,1,Fic);
            break;
        case 1:             // Code Titre
            fread(&STitre,1,1,Fic);
            Titre[STitre]=0;
            fread(Titre,STitre,1,Fic);

            PrintAt(3,posy,"Loading information about %s",Titre);
            posy++;

            if (posy>=45) {
                MoveText(1,4,78,46,1,3);
                posy--;
                ChrLin(1,46,78,32);
                }
            break;
        case 2:             // Code Programmeur
            fread(&SMeneur,1,1,Fic);
            fread(Meneur,SMeneur,1,Fic);
            Meneur[SMeneur]=0;
            break;
        case 3:             // Code Nom du programme
            fread(&SFilename,1,1,Fic);
            Filename[SFilename]=0;
            fread(Filename,SFilename,1,Fic);
            break;
        case 4:             // Checksum
            fread(&Checksum,4,1,Fic);
            break;
        case 5:             // Format
            fread(&format,2,1,Fic);
            app[nbr]=malloc(sizeof(struct player));

            app[nbr]->Filename=malloc(SFilename+1);
            strcpy(app[nbr]->Filename,Filename);

            app[nbr]->Meneur=malloc(SMeneur+1);
            strcpy(app[nbr]->Meneur,Meneur);

            app[nbr]->Titre=malloc(STitre+1);
            strcpy(app[nbr]->Titre,Titre);

            app[nbr]->Checksum=Checksum;
            app[nbr]->ext=format;
            app[nbr]->pres=0;

            app[nbr]->type=KKType;

            nbr++;
            break;
        case 6:             // Fin de fichier
            fin=1;
            break;
        case 7:             // Reset
            Checksum=0;

            strcpy(Titre,"?");
            strcpy(Meneur,"?");
            strcpy(Filename,".");

            SFilename=strlen(Filename);
            STitre=strlen(Titre);
            SMeneur=strlen(Meneur);

            KKType=0;
            break;
        case 8:             // Checksum
            fread(&KKType,1,1,Fic);
            break;
        }
    }
    while(fin==0);
    else return 0;
return 1;
}




void KKR_Search(char *nom2)
{
struct find_t fic;
char moi[256],nom[256];
char ok;
FILE *Fic;

char **TabRec;  // Tableau qui remplace les appels recursifs
int NbrRec;     // Nombre d'element dans le tableau

TabRec=malloc(500*sizeof(char*));
TabRec[0]=malloc(strlen(nom2)+1);
memcpy(TabRec[0],nom2,strlen(nom2)+1);
NbrRec=1;

do
{
strcpy(nom,TabRec[NbrRec-1]);

PrintAt(1,1,"%-78s",nom);

strcpy(moi,nom);
strcat(moi,"*.KKR");

if (_dos_findfirst(moi,63-_A_SUBDIR,&fic)==0)
do
    {
    ok=0;
    if ((fic.attrib&_A_SUBDIR)!=_A_SUBDIR)  {
        strcpy(moi,nom);
        strcat(moi,fic.name);
        Fic=fopen(moi,"rb");
        if (Fic==NULL)  {
            PrintAt(0,0,"KKR_Read (1)");
            exit(1);
            }
        ok=KKR_Read(Fic);
        fclose(Fic);
        }
    }
while (_dos_findnext(&fic)==0);

free(TabRec[NbrRec-1]);
NbrRec--;

strcpy(moi,nom);
strcat(moi,"*.*");

if (_dos_findfirst(moi,_A_SUBDIR,&fic)==0)
do
    {
    if  ( (fic.name[0]!='.') & (((fic.attrib) & _A_SUBDIR) == _A_SUBDIR) )
            {
            strcpy(moi,nom);
            strcat(moi,fic.name);
            strcat(moi,"\\");

            TabRec[NbrRec]=malloc(strlen(moi)+1);
            memcpy(TabRec[NbrRec],moi,strlen(moi)+1);
            NbrRec++;
            }
    }
while (_dos_findnext(&fic)==0);
}
while(NbrRec>0);


free(TabRec);

}


// int Allform[1024];


void Search(char *nom2)
{
struct find_t fic;
char moi[256],nom[256];
short n;
short o;
char ok;
char bill;
signed long wok;

unsigned long KKcrc;
unsigned long K1crc;    // crc calcul� une fois pour toutes

unsigned long C;

char **TabRec;  // Tableau qui remplace les appels recursifs
int NbrRec;     // Nombre d'element dans le tableau

char *StrVerif,Verif;

TabRec=malloc(500*sizeof(char*));
TabRec[0]=malloc(strlen(nom2)+1);
memcpy(TabRec[0],nom2,strlen(nom2)+1);
NbrRec=1;

do
{
o=nbrdir+1;

PrintAt(1,1,"%-78s",nom);
St_Dir++;

strcpy(nom,TabRec[NbrRec-1]);

PrintAt(1,1,"%-78s",nom);

if (_dos_findfirst(nom,63-_A_SUBDIR,&fic)==0)
do
    {
    ok=0;
    wok=-1;

    if ((fic.attrib&_A_SUBDIR)!=_A_SUBDIR)
        {
        C=0;
        KKcrc=0;    // CRC du fichier courant
        K1crc=0;

        for(n=0;n<nbr;n++)
            if ( (!stricmp(fic.name,app[n]->Filename)) & (app[n]->Checksum!=0) )
                {
                if (KKcrc==0)
                    {
                    if (K1crc==0)
                        {
                        strcpy(moi,nom);
                        moi[strlen(moi)-3]=0;
                        strcat(moi,fic.name);
                        crc32file(moi,&KKcrc);
                        K1crc=KKcrc;
                        }
                        else
                        KKcrc=K1crc;

                    if (KKcrc!=app[n]->Checksum) KKcrc=0;
                    }
                }

        Verif=0;
        StrVerif=app[0]->Titre;     // pour pas mettre NULL, ca veut rien dire

        for(n=0;n<nbr;n++)
            {
            bill=0;
            if (!stricmp(fic.name,app[n]->Filename))
                {
                if ( (KKcrc==0) & (app[n]->Checksum==0) )
                    {
                    int x,t;

                    if ( (Verif==0) | (strcmp(StrVerif,app[n]->Titre)!=0) )
                        {
                        strcpy(moi,nom);
                        moi[strlen(moi)-3]=0;
                        strcat(moi,fic.name);

                        x=20;
                        if (strlen(moi)>x) x=strlen(moi);
                        if ((6+strlen(app[n]->Titre))>x) x=(strlen(app[n]->Titre)+6);
                        if (strlen(app[n]->Meneur)>x) x=strlen(app[n]->Meneur);

                        if (x>78) x=78;
                        t=(80-x)/2;

                        SaveEcran();

                        WinCadre(t-1,9,t+x,15,2);
                        ColWin(t,10,t+x-1,14,10*16+2);
                        ChrWin(t,10,t+x-1,14,32);
                        PrintAt(t,10,"Do you think that");
                        PrintAt(t,11,"%s",moi);
                        PrintAt(t,12,"is %s of",app[n]->Titre);
                        PrintAt(t,13,"%s",app[n]->Meneur);
                        PrintAt(t,14,"(Y/N)");

                        do
                            {
                            t=Wait(0,0,0);
                            }
                        while ( (t!='y') & (t!='Y') & (t!='n') & (t!='N'));

                        ChargeEcran();
                        if ( (t=='y') | (t=='Y') )
                            {
                            StrVerif=app[n]->Titre;
                            Verif=1;
                            }
                            else
                            Verif=2;
                        }

                    if (Verif==1)
                        {
                        app[n]->pres=o;   // l'appl. n est presente dans le dir. o
                        bill=1;
                        }
                    }

                if ( (KKcrc==app[n]->Checksum) & (KKcrc!=0) )
                    {
                    app[n]->pres=o;
                    bill=1;
                    }

                if (bill==1)
                    {
                    strcpy(moi,nom);
                    moi[strlen(moi)-3]=0;
                    ok=1;
                    wok=n;
                    }
                }
            }
        }

    if (ok==1)
        {
        St_App++;

        nbrdir=o;
        strcpy(dir[o-1],moi);

        PrintAt(3,posy,"Found %s in %s",app[wok]->Titre,dir[o-1]);
        posy++;

        if (posy>=45)
            {
            MoveText(1,4,78,46,1,3);
            posy--;
            ChrLin(1,46,78,32);
            }
        }
   }
while (_dos_findnext(&fic)==0);

free(TabRec[NbrRec-1]);
NbrRec--;

if (_dos_findfirst(nom,_A_SUBDIR,&fic)==0)
do
	{
    if  ( (fic.name[0]!='.') & (((fic.attrib) & _A_SUBDIR) == _A_SUBDIR) )
			{
			strcpy(moi,nom);
			moi[strlen(moi)-3]=0;
            strcat(moi,fic.name);
			strcat(moi,"\\*.*");

            TabRec[NbrRec]=malloc(strlen(moi)+1);
            memcpy(TabRec[NbrRec],moi,strlen(moi)+1);
            NbrRec++;
			}
	}
while (_dos_findnext(&fic)==0);

}
while(NbrRec>0);


free(TabRec);
}


char TestCar(char c)
{
if (c==32) return 1;
if (c=='=') return 1;
return 0;
}

void ClearAllSpace(char *name)
{
char c,buf[128];
short i,j;

i=0;    // navigation dans name
j=0;    // position dans buf

while ( (TestCar(name[i])) & (name[i]!=0) ) i++;

if (name[i]!=0)
    while ((c=name[i])!=0)
        {
        buf[j]=name[i];
        j++;
        i++;
        if (TestCar(name[i]))
            {
            c=name[i];
            while ( (TestCar(name[i])) & (name[i]!=0) )
                {
                if (name[i]=='=') c='=';
                i++;
                }
            buf[j]=c;
            j++;
            }
        }
buf[j]=0;

strcpy(name,buf);
}

int Traite(char *from,char *to)
{
int i;

i=0;

while( (from[i]!=0) & (from[i]!='=') )
    {
    to[i]=from[i];
    i++;
    }
to[i]=0;

if (from[i]!=0)
    {
    sscanf(from+i+1,"%d",&i);
    return i;
    }
return 0;
}



void ConfigFile(void)
{
FILE *fic;
char chaine[128];

char var[128];
int valeur;

char erreur;

fic=fopen("kk_setup.ini","rt");
if (fic==NULL) return;

SaveEcran();


while (fgets(chaine,128,fic)!=NULL)
{
ChrLin(0,0,80,32);
erreur=1;
ClearAllSpace(chaine);

if (chaine[0]==';') erreur=0;

if (chaine[0]=='\n') erreur=0;

valeur=Traite(chaine,var);

if (!stricmp(var,"mask"))
    {
    Cfg->wmask=valeur;
    erreur=0;
    }

if (!stricmp(var,"vsize"))
    {
    Cfg->TailleY=valeur;
    erreur=0;
    }

if (!stricmp(var,"wintype"))
    {
    Cfg->fentype=valeur;
    erreur=0;
    }

if (!stricmp(var,"ansispeed"))
    {
    Cfg->AnsiSpeed=valeur;
    erreur=0;
    }

if (!stricmp(var,"ssaverspeed"))
    {
    Cfg->SaveSpeed=valeur;
    erreur=0;
    }


if (erreur==1)
    {
    WinError(chaine);
    }
}

ChargeEcran();
}

