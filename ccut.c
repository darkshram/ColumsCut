/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#include "common.h"
#include <ctype.h>


#define OPT_NAME  1
#define OPT_SHORT 2
#define OPT_LONG  3

#define FIELD_INCLUDED 1
#define FIELD_LAST 2

#define DELIM_PREFIX 1
#define DELIM_POSTFIX 2

#define END_OF_LINE -1

//UTF-8 chars always have the top bit (128) set
#define isUTF8ch(ch) (((ch) & 128) ? 1 : 0)

char *Version="3.1";


char *Delim=NULL, *OutputDelim=NULL, *OutPath=NULL, *FieldSpec=NULL;
int StartPos=0, EndPos=0, MaxField=0, DelimLen=0;

char **FilePaths=NULL;


//Variable Names used with the '-V' option
char **VarNames=NULL;
int VarCount=0;

//this is defined in common.c
//int Flags=0;


typedef struct 
{
int Flags;
const char *Start;
const char *End;
} TCutField;





void DisplayHelp()
{
printf("Usage: cut OPTION... [FILE]...\n");
printf("Print selected parts of lines from each FILE to standard output.\n");
printf("Supports: Multiple delimiters. Quoting within the document. Outputing fields in anyspecified order. Using a different delimiter on output than input.\n");
printf("\n");
printf("Mandatory arguments to long options are mandatory for short options too.\n");
printf("  -b, --bytes=[list]      select only these bytes\n");
printf("  -c, --characters=[list] select only these characters\n");
printf("  -d, -t, --delimiter=[list] list of delimiter characters. Default is just the 'tab' character.\n");
printf("  -D, --delimstr=[delim] use a string as a delimiter rather than a list of single-byte ones. Only one string delimiter can be used and it cannot be used in combination with -d or -t options\n");
printf("  -e  list of delimiter characters, but allowing quoted values like in 'echo -e'. See 'Quoted Delimiters' below\n");
printf("  -E  like '-D' but allowing quoted values like in 'echo -e'. See 'Quoted Delimiters' below\n");
printf("  -f, --fields=LIST       select only these fields;  also print any line without delimiter characters, unless the -s option is specified\n");
printf("      --complement        complement the set of selected bytes, characters or fields\n");
printf("  -j, --join-delims       combine runs of delimters and treat them as one delimiter\n");
printf("  -q, --quote             honor quoting within target document using \\ or ' or \"\n");
printf("  -Q, --quote-strip       honor quoting within target document, but strip quotes off output fields\n");
//printf("  -r, --reverse           cut by counting chars/bytes/fields from end of line, not from start of line\n");
printf("  -s, --only-delimited    do not print lines not containing delimiters\n");
printf("      --utf8              honor UTF-8 characters in input\n");
printf("  -V, --vars=NAMES        print out bash commands to set variables using the supplied list of names\n");
printf("  -T, --output-delimiter=[string] use string as the output delimiter\n");
printf("                            the default is to use the input delimiter\n");
printf("  -z, --zero-terminated   read input where lines are null terminated\n");
printf("  -?  --help     display this help and exit\n");
printf("  -v  --version  output version information and exit\n");
printf("\n");
printf("Use one, and only one of -b, -c or -f.  Each LIST is made up of one range, or many ranges separated by commas.\n\n");
printf("If -d is used, then all characters in the next argument are treated as individual delimiters. If -D is used, then they are treated as a single string delimiter.\n\n");
printf("Character set support: THIS CUT DOES NOT SUPPORT 16-BIT WIDE CHARACTERS (yet). So '-c' and '-b' are equivalent. It does support UTF-8 characters via the --utf8 switch, but this overrides -b, so again -b and -c are equivalent\n\n");


printf("Quoted Delimiters: If the '-e' option is used rather than '-d' or '-E' rather than '-D' then the following quoted characters are recognized:\n");
printf("	\\\\		backslash\n");
printf("	\\e		escape\n");
printf("	\\t		tab\n");
printf("	\\r		carriage-return\n");
printf("	\\n		newline\n");
printf("	\\xnn		where 'nn' is a two-digit hex-code\n\n");
printf("You will probably need to protect such quoted characters from the shell using quotes, or it may replace the '\\' character, turning, for example, '\\n' into just 'n'\n\n");

printf("Selected input is written in the SPECIFIED ORDER (unlike gnu cut), and fields can be output multiple times.\n");
printf("However, order has no meaning when cut is run with --complement, so then fields are output in the order they are encountered in the data\n");
printf("\nEach range is one of:\n");
printf("  N     N'th byte, character or field, counted from 1\n");
printf("  N-    from N'th byte, character or field, to end of line\n");
printf("  N-M   from N'th to M'th (included) byte, character or field\n");
printf("  -M    from first to M'th (included) byte, character or field\n");
printf("reversed ranges (e.g. 5-2) are supported\n\n");
printf("With no FILE, or when FILE is -, read standard input.\n\n");

printf("The '-V' or '--vars' option allows a comma-separated list of variable names to be supplied. Cut will then match output fields to those variable names and print out commands to set those variables in a borne-style shell. This can then be used with the 'eval' command, like so:\n\n");
printf("	eval `echo apples,oranges,pears,lemons,lime | ccut -d , -f 2,4,5,1,3 -V citrus1,citrus2,citrus3,poma1,poma2`\n\n");
printf("This will result in the variables being set in the shell, citrus1=oranges, citrus2=lemons, citrus3=limes, poma1=apples and poma2=pears\n");

printf("\n");
printf("Report bugs to colums.projects@gmail.com, or at https://github.com/ColumPaget/ColumsCut\n");
printf("Thanks to https://github.com/larrypl and https://github.com/schragge for bug reports\n");

exit(0);
}


void DisplayVersion()
{
printf("ccut (Colum's cut) %s\n",Version);
printf("Copyright (C) 2015 Colum Paget\n");
printf("License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n");
printf("This is free software: you are free to change and redistribute it.\n");
printf("There is NO WARRANTY, to the extent permitted by law.\n");

printf("\nWritten by Colum Paget\n");

exit(0);
}



void SetupVarNames(TCutField *CutFields, int MaxField, char *Arg)
{
int i=0;
char *ptr;

//must be at least one arg, or we'd not have been called
VarCount=1;

ptr=strchr(Arg, ',');
while (ptr)
{
VarCount++;
ptr=strchr(ptr+1, ',');
}

if (VarCount > 0)
{
	VarNames=calloc(VarCount+1, sizeof(char *));
	ptr=strtok(Arg, ",");
	while (ptr)
	{
		VarNames[i]=CopyStr(VarNames[i], ptr);
		i++;
		ptr=strtok(NULL, ",");
	}
}

}


void ValidateOptions()
{
if (! (Flags & (FLAG_FIELDS | FLAG_CHARS | FLAG_BYTES))) 
{
	fprintf(stderr,"cut: you must specify a list of bytes, characters, or fields\n");
	exit(3);
}

if (Flags & FLAG_SETVARS)
{
	if (! (Flags & FLAG_FIELDS))
	{
	fprintf(stderr,"cut: the 'vars' option requires the 'fields' (-f) option\n");
	exit(3);
	}
}

}


TCutField *ParseCommandLine(int argc, char *argv[])
{
int i, State=OPT_NAME;
char *VarNamesArg=NULL, *ptr, *arg;
char *Tempstr=NULL;
TCutField *CutFields=NULL;
int val, fcount=0;

for (i=1; i < argc; i++)
{
	State=OPT_NAME;
	ptr=argv[i];
	while (*ptr=='-') 
	{
		State++;
		ptr++;
	}

	switch (State)
	{
		case OPT_NAME:
			FilePaths=(char **) realloc(FilePaths, sizeof(char *) * (fcount + 1));
			FilePaths[fcount]=argv[i];
			fcount++;
		break;


		case OPT_SHORT:
			switch (*ptr)
			{
				case 'c': 
					Flags |= FLAG_CHARS; 
					ParseCommandValue(argc, argv, ++i, 0, &FieldSpec);
					GetMinMaxFields(FieldSpec, &val, &MaxField);
				break;

				case 'b': 
					Flags |= FLAG_BYTES; 
					ParseCommandValue(argc, argv, ++i, 0, &FieldSpec);
					GetMinMaxFields(FieldSpec, &val, &MaxField);
				break;

				//not working yet
				//case 'r': Flags |= FLAG_REVERSE; break;
				case 's': Flags |= FLAG_ONLY_DELIM_LINES; break;
				case 'q': Flags |= FLAG_QUOTED; break;
				case 'Q': Flags |= FLAG_QUOTED | FLAG_QUOTE_STRIP; break;
				case 'j': Flags |= FLAG_COMBINE_DELIMS; break;
				case 'v': DisplayVersion(); break;

				//list of variable names. This option makes ccut print out 
				//a string of <varname>=<field>; that can be used with the shell 'eval'
				//command to set variables in the shell
				case 'V': 
					Flags |= FLAG_SETVARS; 
					ParseCommandValue(argc, argv, ++i, 0, &VarNamesArg);
				break;


				case 'f':
					Flags |= FLAG_FIELDS;
					ParseCommandValue(argc, argv, ++i, 0, &FieldSpec);
					GetMinMaxFields(FieldSpec, &val, &MaxField);
				break;

				case 'e':
						ParseCommandValue(argc, argv, ++i, FLAG_QDELIM, &Tempstr);
						Delim=CatStr(Delim, Tempstr);
				break;

				case 'd':
				case 't':
						ParseCommandValue(argc, argv, ++i, 0, &Tempstr);
						Delim=CatStr(Delim, Tempstr);
				break;

				case 'D':
					if (StrLen(Delim) !=0) fprintf(stderr,"WARN: ignoring -D option, as delimiter set by previous option. Only one string delimiter is allowed and cannot be combined with -d or -t\n");
					else 
					{
						ParseCommandValue(argc, argv, ++i, 0, &Delim);
						Flags |= FLAG_DELIMSTR;
					}
				break;

				case 'E':
					if (StrLen(Delim) !=0) fprintf(stderr,"WARN: ignoring -E option, as delimiter set by previous option. Only one string delimiter is allowed and cannot be combined with -d or -t\n");
					else 
					{
						ParseCommandValue(argc, argv, ++i, FLAG_QDELIM, &Delim);
						Flags |= FLAG_DELIMSTR;
					}
				break;


				case 'T':
					ParseCommandValue(argc, argv, ++i, FLAG_REPLACE_DELIM, &OutputDelim);
				break;

				case 'z': Flags |= FLAG_ZERO_TERM; break;

				default:
					fprintf(stderr,"ERROR: unknown short option '-%c'\n",*ptr);
				case '?':
					DisplayHelp();
				break;
			}
		break;

		case OPT_LONG:
				arg=strchr(ptr,'=');
				if (arg) 
				{
					*arg='\0';
					arg++;
				}

				switch (*ptr)
				{
					case 'b':
						if (strcmp(ptr, "bytes")==0)
						{
							Flags |= FLAG_BYTES;
							FieldSpec=CopyStr(FieldSpec, arg);
							GetMinMaxFields(FieldSpec, &val, &MaxField);
						}
					break;

					case 'c':
						if (strcmp(ptr, "characters")==0) 
						{
							Flags |= FLAG_CHARS;
							FieldSpec=CopyStr(FieldSpec, arg);
							GetMinMaxFields(FieldSpec, &val, &MaxField);
						}
						else if (strcmp(ptr, "complement")==0) Flags |= FLAG_COMPLEMENT;
					break;

					case 'd':
					if (strcmp(ptr,"delimiter")==0) 
					{
						ParseCommandValue(argc, argv, ++i, 0, &Tempstr);
						Delim=CatStr(Delim, Tempstr);
					}
				else if (strcmp(ptr, "delimstr")==0)
				{
					if (StrLen(Delim) !=0) fprintf(stderr,"WARN: ignoring --delimstr option, as delimiter set by previous option. Only one string delimiter is allowed and cannot be combined with -d or -t\n");
					else 
					{
						ParseCommandValue(argc, argv, ++i, 0, &Delim);
						Flags |= FLAG_DELIMSTR;
					}
				}
					break;

					case 'f':
						if (strcmp(ptr, "fields")==0)
						{
							Flags |= FLAG_BYTES;
							FieldSpec=CopyStr(FieldSpec, arg);
							GetMinMaxFields(FieldSpec, &val, &MaxField);
						}
					break;


					case 'o':
						if (strcmp(ptr, "only-delimited")==0) Flags |= FLAG_ONLY_DELIM_LINES;
						if (strcmp(ptr, "output-delimiter")==0) ParseCommandValue(argc, argv, ++i, FLAG_REPLACE_DELIM, &OutputDelim);
					break;

					case 'j':
						if (strcmp(ptr, "join-delims")==0) Flags |= FLAG_COMBINE_DELIMS;
					break;

					/*
					//not working yet
					case 'r':
						if (strcmp(ptr, "reverse")==0) Flags |= FLAG_REVERSE;
					break;
					*/

					case 'q':
						if (strcmp(ptr,"quote")==0) Flags |= FLAG_QUOTED; 
						if (strcmp(ptr,"quote-strip")==0) Flags |= FLAG_QUOTED | FLAG_QUOTE_STRIP; 
					break;

					case 'v':
						if (strcmp(ptr,"version")==0) DisplayVersion();
						if (strcmp(ptr, "vars")==0)
						{
							Flags |= FLAG_SETVARS; 
							ParseCommandValue(argc, argv, ++i, 0, &VarNamesArg);
						}
					break;

					case 'u':
						if (strcmp(ptr,"utf8")==0) Flags |= FLAG_UTF8;
					break;

					case 'z': 
						if (strcmp(ptr,"zero-terminated")==0) Flags |= FLAG_ZERO_TERM; 
					break;

					default:
					fprintf(stderr,"ERROR: unknown long option '--%s'\n",ptr);
					case 'h':
						if (strcmp(ptr,"help")==0) DisplayHelp();
					break;


				}
		break;

		default:
		break;
	}
}

if ((Flags & FLAG_FIELDS) && (StrLen(Delim)==0)) Delim=CopyStr(Delim,"	");
CutFields=(TCutField *) calloc(MaxField+1,sizeof(TCutField));

DelimLen=StrLen(Delim);


//NULL terminate FilePaths array
if (FilePaths)
{
	FilePaths=(char **) realloc(FilePaths, sizeof(char *) * (fcount + 1));
	FilePaths[fcount]=NULL;
}


if (Flags & FLAG_SETVARS) SetupVarNames(CutFields, MaxField, VarNamesArg);


Destroy(Tempstr);
Destroy(VarNamesArg);

return(CutFields);
}


int isDelimeter(const char *Chars)
{
//end of string is a special case. We treat it as a delimiter but
//return a special value that can be checked by functions wanting
//to discrimitate between a delimiter and the end of line
if (*Chars=='\0') return(END_OF_LINE);

if (Flags & FLAG_DELIMSTR)
{
	if (strncmp(Delim, Chars, DelimLen)==0) 
	{
		if (! (Flags & FLAG_REPLACE_DELIM)) OutputDelim=CopyStrLen(OutputDelim, Chars, DelimLen);
		return(DelimLen);
	}
}
else if (memchr(Delim,*Chars,DelimLen)) 
{
	if (! (Flags & FLAG_REPLACE_DELIM)) OutputDelim=CopyStrLen(OutputDelim, Chars, 1);
	return(1);
}

return(0);
}


const char *ConsumeDelimiters(const char *str)
{
const char *ptr;
int len;

ptr=str;
len=isDelimeter(ptr);
while (len > 0)
{
ptr+=len;
len=isDelimeter(ptr);
}

return(ptr);
}


//Advance past any quoted strings
const char *SkipQuotedChars(const char *Chars)
{
const char *ptr;
char qchar;

ptr=Chars;
switch(*ptr)
{
	//for ' and " quotes, advance to the next ' or "
	case '\'':
	case '"':
		qchar=*ptr;
		ptr++;
		while ((*ptr != '\0') && (*ptr != qchar)) ptr++;
		if (*ptr==qchar) ptr++;
	break;

	//for backslash quoting, just skip quoted char
	case '\\':
		ptr++;
		if (*ptr !='\0') ptr++;
	break;
}

return(ptr);
}


const char *ExtractNextField(const char *Line, const char **Start, const char **End)
{
const char *ptr, *fstart;
int len;

fstart=Line;
//end-of-line is handled via isDelimeter
for (ptr=fstart; ; ptr++)
{
	//if current character is one of the delimiters
	if (Flags & FLAG_QUOTED) ptr=SkipQuotedChars(ptr);

	len=isDelimeter(ptr);
	if (len)
	{
		*Start=fstart;
		*End=ptr;
		break;
	}
}

return(ptr);
}



const char *ExtractFieldHandleDelimiter(const char *Str, int *fcount)
{
const char *ptr;
int len;

	ptr=Str;

	//we only count a field if we found a delimiter, this is so the '-s' or
	//--only-delimiter options can work. Thus we have to check if isDelimeter
	//returns TRUE rather than END_OF_LINE
	len=isDelimeter(ptr);
	switch (len)
	{
	case END_OF_LINE:
	/*don't increment ptr, nor count a field*/
	break;

	case 0:
	ptr++;
	break;

	default:
 	(*fcount)++;
	ptr+=len;
	break;
	}


	if (Flags & FLAG_COMBINE_DELIMS) ptr=ConsumeDelimiters(ptr);

	return(ptr);
}


int ExtractFields(const char *Line, TCutField *Fields)
{
int fcount=0, len;
const char *ptr;

ptr=Line;
while (fcount < MaxField)
{
	ptr=ExtractNextField(ptr, &(Fields[fcount].Start), &(Fields[fcount].End));
	ptr=ExtractFieldHandleDelimiter(ptr, &fcount);
	if (*ptr=='\0') break;
}

//We have to count the remaining fields, even though we might not be outputting them, 
//because the '--reverse' option requires us to know the total number of fields
while (*ptr !='\0')
{
	ptr=ExtractFieldHandleDelimiter(ptr, &fcount);
}

return(fcount);
}



void OutputDelimiter(char FoundDelim)
{
		if (Flags & FLAG_REPLACE_DELIM) fputs(OutputDelim, stdout);
		else if (Flags & FLAG_DELIMSTR) fputs(Delim, stdout);
		else if (FoundDelim !=0) fputc(FoundDelim, stdout);
		else if (StrValid(Delim)) fputc(*Delim, stdout);
}


void OutputField(const char *start, const char *end, int DelimFlags)
{
const char *ptr;
char delim;
static int OutputNo=0;


if (start)
{
	//safety check in case end was never set
	for (ptr=start; ptr && (ptr < end) ; ptr++) 
	{
		if (*ptr == '\0') break;
	}

	//we try to use the delimiter that we found, because 
	//ccut uses multiple delimiters, so we use the one that's
	//been encountered as the output delimiter too
	if (DelimFlags & DELIM_PREFIX) delim=*(start-1);
	else delim=*ptr;

	//the last field will likely have no delimiter. However, because we can rearrange
	//delimiters it might need one in the output Thus we use the first delimiter in
	//the given delimiter list to cover this scenario
	if (delim=='\0') delim=*Delim;

	if (
			(Flags & FLAG_QUOTE_STRIP) &&
			((*start=='\'') || (*start=='"')) &&
			(ptr > start) &&
			(*(ptr-1)==*start)
		)
	{
		start++;
		ptr--;
	}

	if (Flags & FLAG_SETVARS)
	{
		if (OutputNo < VarCount)
		{
		fputs(VarNames[OutputNo], stdout);
		fputs("='", stdout);
		fwrite(start,ptr-start,1,stdout);
		fputs("'; ", stdout);	
		}
	}
	else
	{
		//if we're outputing fields in reverse order than we copy a delimiter to the start
		if (DelimFlags & DELIM_PREFIX) OutputDelimiter(delim); 
    //if (Flags & FLAG_DELIMSTR) 
		fwrite(start,ptr-start,1,stdout);

		//if we're outputing fields in normal order than we copy a delimiter to the end
		if (DelimFlags & DELIM_POSTFIX) OutputDelimiter(delim); 
	}
}
//Use the cached 'last delim' if the field doesn't exist
else if (DelimFlags) OutputDelimiter(0); 

OutputNo++;
}



void OutputCutField(int FCount, TCutField *CutFields, int FieldNo, int DelimFlags)
{

if (FieldNo > 0)
{
//	if (Flags & FLAG_REVERSE) FieldNo=FCount+1-FieldNo;
	FieldNo--; //Fields are 1 based, so make zero based

	OutputField(CutFields[FieldNo].Start, CutFields[FieldNo].End, DelimFlags);
}

}


//Output fields in range, like 'cut -f 2-5'
void OutputFieldRange(int FCount, TCutField *CutFields,int Start, int End, int IsLast)
{
int i;

	if (Start < 1) Start=1;

//reverse order! Except if End==0 that means no end position has been given, like 2- which means 'to the end' 
if (Start > End)
{
	for (i=Start; i >= End; i--) 
	{
    if (i == Start) OutputCutField(FCount,CutFields,i, FALSE);
    else if ((i == End) && (! IsLast)) OutputCutField(FCount, CutFields, i, DELIM_PREFIX | DELIM_POSTFIX);
		else OutputCutField(FCount,CutFields,i, DELIM_PREFIX);
	}
}
else
{
	for (i=Start; i <= End; i++) 
	{
		if (IsLast && (i == End)) 
		{
				OutputCutField(FCount,CutFields,i, FALSE);
		}
		else OutputCutField(FCount,CutFields,i, DELIM_POSTFIX);
	}
}

}


//Output all fields after a field. This is a 'range' command but with no last field
//given. So like this:
//cut -d , -f 2-

void OutputFieldsAfter(int FCount,TCutField *CutFields, int First, int IsLast)
{
const char *ptr, *Start, *End;

if (First < 1) First=1;
//Field Numbers are 1-based, but need to be converted to zero-base
First--;
ptr=CutFields[First].Start;

while (ptr && (*ptr !='\0'))
{
	ptr=ExtractNextField(ptr, &Start, &End);
	if (IsLast && (*ptr=='\0')) OutputField(Start, End, FALSE);
	else OutputField(Start, End, DELIM_POSTFIX);
	if (*ptr !='\0') ptr++;
}

}




void OutputRequestedFields(int FCount, TCutField *CutFields, const char *FieldSpec)
{
char *ptr;
long Start=0, End=0;
int LastField=FALSE;


//ptr has to be 'char *' rather than 'const char *' to keep strtol happy
ptr=(char *) FieldSpec;
while (*ptr != '\0')
{
	switch (*ptr)
	{
		case '-': 
		ptr++;
		if (*ptr == '\0') End=0;
		else End=strtol(ptr,&ptr,10);
		if (*ptr=='\0') LastField=TRUE;
		if (End==0) OutputFieldsAfter(FCount, CutFields, Start, LastField);
		else OutputFieldRange(FCount, CutFields, Start, End, LastField);
		Start=-1;
		break;
	
		case ',':
		if (Start > -1) OutputCutField(FCount,CutFields,Start,DELIM_POSTFIX);
		Start=-1;
		ptr++;
		break;
	
		default:
		Start=strtol(ptr,&ptr,10);
		break;
	}
}

//if there's still a field in hand, output that
if (Start > -1) OutputCutField(FCount,CutFields,Start,FALSE);

putchar('\n');
}



//Bit of a complex function to output the fields NOT specified on the command line.
//This function parses the field specification passed on the command-line, and marks
//which fields are included. It then runs through the input line, and outputs any fields
//that haven't been marked.
void OutputComplementFields(int FCount, TCutField *CutFields, const char *FieldSpec, const char *Line)
{
const char *ptr;
//we set start to -1 to distinguish between 'no field' or 'first field'
long Start=-1, End=-1, i;
const char *fstart, *fend;
int DelimFlags=0;

//ptr has to be 'char *' rather than 'const char *' to keep strtol happy
ptr=(char *) FieldSpec;
while (*ptr != '\0')
{
	switch (*ptr)
	{
		case '-': 
		ptr++;

		if (*ptr == '\0') End=-1;
		else End=strtol(ptr, (char **) &ptr, 10) - 1;

		if (Start==-1) Start=0;


		if (End==-1) CutFields[Start].Flags=FIELD_INCLUDED | FIELD_LAST;
		else 
		{
			if (Start > End)
			{
				i=Start;
				Start=End;
				End=i;
			}

			for (i=Start; i <= End; i++) CutFields[i].Flags=FIELD_INCLUDED;
		}
		Start=-1;
		break;
	
		case ',':
		if (Start > -1) CutFields[Start].Flags=FIELD_INCLUDED;
		Start=-1;
		ptr++;
		break;
	
		default:
		Start=strtol(ptr, (char **) &ptr, 10) - 1;
		break;
	}
}


if (Start > -1) CutFields[Start].Flags=FIELD_INCLUDED;

i=0;
ptr=Line;
while (*ptr !='\0')
{
	ptr=ExtractNextField(ptr, &fstart, &fend);
	if (*ptr =='\0') DelimFlags=0;
	else DelimFlags=DELIM_POSTFIX;

	if (i < MaxField)
	{
		if (! (CutFields[i].Flags & FIELD_INCLUDED)) 
		{
			OutputField(fstart,fend, DelimFlags);
		}
		if (CutFields[i].Flags & FIELD_LAST) break;
	}
	else 
	{
		OutputField(fstart,fend, DelimFlags);
	}

	i++;
	if (*ptr !='\0') ptr++;
}


}


void OutputFields(TCutField *CutFields, const char *FieldSpec, const char *Line)
{
int count;

		count=ExtractFields(Line,CutFields);
		if (count==0)
		{
			if (Flags & FLAG_ONLY_DELIM_LINES) /* Do Nothing */ ;
			else fwrite(Line,StrLen(Line),1,stdout);
		}
		else 
		{
			if (Flags & FLAG_COMPLEMENT) OutputComplementFields(count, CutFields, FieldSpec, Line);
			else OutputRequestedFields(count,CutFields,FieldSpec);
		}
}


int UTF8Position(const char *Line, int pos)
{
int count=0, val;
const char *ptr;

for (ptr=Line; *ptr !='\0'; )
{
	count++;
	if (count > pos) break;

	//utf-8 uses the top bit set to distinguish from 7-bit ascii
	//if the 1st 2 bits are set (128+64==192) then it's a 2-byte code
	//if the 1st 3 bits are set (128+64+32==224) then it's a 3-byte code
	//and if the 1st 4 bits, then a 4-byte code
	val=*ptr & 224;
	switch (val)
	{
	case 192:
	ptr++;
	if (! isUTF8ch(*ptr)) fprintf(stderr, "top bit not set in expected utf code. Possible corrupt input\n");
	break;

	case 224:
	if ((*ptr) & 16)
	{
	ptr++;
	if (! isUTF8ch(*ptr)) fprintf(stderr, "top bit not set in expected utf code. Possible corrupt input\n");
	}
	ptr++;
	if (! isUTF8ch(*ptr)) fprintf(stderr, "top bit not set in expected utf code. Possible corrupt input\n");
	ptr++;
	if (! isUTF8ch(*ptr)) fprintf(stderr, "top bit not set in expected utf code. Possible corrupt input\n");
	break;

	}
	ptr++;
}

return(ptr-Line);
}


void OutputSubstr(const char *Line, int Start, int End)
{
int uStart, uEnd;

if (Flags & FLAG_UTF8)
{
uStart=UTF8Position(Line, Start);
uEnd=UTF8Position(Line, End);
fwrite(Line+uStart,uEnd-uStart,1,stdout);
}
else fwrite(Line+Start,End-Start,1,stdout);
}


void OutputBytes(const char *Line, TCutField *Fields)
{
char *ptr;
int len;
long Start=0, End=0;

len=StrLen(Line);
//if (Flags & FLAG_REVERSE) FieldNo=FCount+1-FieldNo;

ptr=FieldSpec;
while (*ptr != '\0')
{
	switch (*ptr)
	{
		case '-': 
			ptr++;
			if (isdigit(*ptr)) End=strtol(ptr,&ptr,10);
			else End=len;
			if ((End < 0) || (End > len)) End=len;
			if (Start < 0) Start=0;
			OutputSubstr(Line, Start, End);
			Start=-1;
		break;
	
		case ',':
			if (Start > -1) OutputSubstr(Line, Start, 1);
			Start=-1;
			ptr++;
		break;
	
		default:
			Start=strtol(ptr,&ptr,10)-1;
		break;
	}
}

if (Start > -1) OutputSubstr(Line, Start, 1);

putchar('\n');
}


void ProcessInput(FILE *InF, TCutField *CutFields)
{
char *Tempstr=NULL;

if (Flags & FLAG_ZERO_TERM) Tempstr=FILEReadLine(Tempstr,InF,'\0');
else Tempstr=FILEReadLine(Tempstr,InF,'\n');

while (Tempstr)
{
	StripTrailingWhitespace(Tempstr);
	if (Flags & FLAG_FIELDS) OutputFields(CutFields,FieldSpec,Tempstr);
	else if (Flags & FLAG_BYTES) OutputBytes(Tempstr, CutFields);
	else if (Flags & FLAG_CHARS) OutputBytes(Tempstr, CutFields);

if (Flags & FLAG_ZERO_TERM) Tempstr=FILEReadLine(Tempstr,InF,'\0');
else Tempstr=FILEReadLine(Tempstr,InF,'\n');
}

Destroy(Tempstr);
}


int main(int argc, char *argv[])
{
TCutField *CutFields=NULL;
FILE *InF;
int i;

CutFields=ParseCommandLine(argc, argv);

if (FilePaths)
{
	for (i=0; FilePaths[i] != NULL; i++)
	{
		InF=fopen(FilePaths[i],"r");
		if (InF) 
		{
			ProcessInput(InF, CutFields);
			fclose(InF);
		}
		else fprintf(stderr, "ERROR: Failed to open [%s]\n",FilePaths[i]);
	}
}
else ProcessInput(stdin, CutFields);

return(0);
}
