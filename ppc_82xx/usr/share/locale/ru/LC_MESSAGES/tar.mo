��       �     D   �  l      8  -  9  �  g   �  �     �  -  �   #  �  7  �          $   �  1   "  �   $  �          #   #  <   "  `     �     �   ,  �     �   %     ,  (   -  U      �   &  �     �     �               '     ?   "  R   5  u   0  �     �   0  �     *   !  F   (  h   +  �   !  �   #  �   "       &     @   "  Y     |     �      �   ,  �     �           /     E   !  `   '  �   ,  �     �   {  �   2  e     �   ,  �     �  Y  �     ;     U     m     �     �     �     �     �     �     	   +  &     R     h     �   1  �   )  �   /  �   2      4   Q   (   �   1   �   %   �   0  !   1  !8     !j   *  !�     !�     !�     !�   %  !�     "	     "     "6     "O     "\     "t     "�   %  "�     "�     "�   &  "�     #     #)     #C     #c     #�   -  #�     #�     #�     #�     #�     $     $/     $I   3  $]   .  $�     $�   
  $�     $�     $�     %	   #  %)     %M     %g     %|     %�     %�     %�  (  %�  V  &�  �  (&   �  *�     +?  Y  -@   3  /�  �  /�     1v     1�   �  1�     2Y   $  2y     2�     2�   '  2�   4  2�     3#     3@   0  3Y     3�   )  3�   0  3�   1  4     44   *  4S     4~     4�     4�     4�     4�     4�   3  5   -  5;   ?  5i   $  5�   <  5�   2  6   $  6>   5  6c   3  6�   "  6�   '  6�   5  7   1  7N   #  7�   .  7�   +  7�     7�   (  8   '  8?     8g      8z     8�     8�   ,  8�   2  9   )  97     9a   x  9y   8  9�     :+   5  :<     :r  �  :�     <
     <)     <F     <d     <{   %  <�     <�     <�     <�     =   .  =)     =X   %  =n     =�   1  =�   )  =�   /  >   2  >3   4  >f   (  >�   ?  >�   ,  ?   3  ?1   ?  ?e     ?�   9  ?�     ?�   
  @     @"   $  @A     @f   *  @{   '  @�     @�     @�     A   "  A      AC   $  A^   (  A�   A  A�     A�   #  A�   ,  B"   3  BO     B�   3  B�     B�   	  B�     B�     C     C,   "  CL     Co   2  C�   +  C�     C�   	  C�     C�     D     D-   +  DI     Du   "  D�     D�     D�     D�     D�      5   ?      m          M   }   O                 J              j       &       d   /   u      l      N   D      >   1   c   �       a       Y      	   "           T       _   A   g   I   �                  o      #   [   U   !           t   '   Z   %   :       W   9   i   R   4   G       x   \   )      E              k   *   =                     V       �      ]   �      2   F   p          v      b   {       
      z   Q   �   P              `   n   $   y   h          @       <   +           s              �   w   r       6          f              L       .   |   0   H   (   7   ~              S      3   X   C   8   B   -   K   q   ^          e           ,   ;            
Device blocking:
  -b, --blocking-factor=BLOCKS   BLOCKS x 512 bytes per record
      --record-size=SIZE         SIZE bytes per record, multiple of 512
  -i, --ignore-zeros             ignore zeroed blocks in archive (means EOF)
  -B, --read-full-records        reblock as we read (for 4.2BSD pipes)
 
Device selection and switching:
  -f, --file=ARCHIVE             use archive file or device ARCHIVE
      --force-local              archive file is local even if has a colon
      --rsh-command=COMMAND      use remote COMMAND instead of rsh
  -[0-7][lmh]                    specify drive and density
  -M, --multi-volume             create/list/extract multi-volume archive
  -L, --tape-length=NUM          change tape after writing NUM x 1024 bytes
  -F, --info-script=FILE         run script at end of each tape (implies -M)
      --new-volume-script=FILE   same as -F FILE
      --volno-file=FILE          use/update the volume number in FILE
 
If a long option shows an argument as mandatory, then it is mandatory
for the equivalent short option also.  Similarly for optional arguments.
 
Informative output:
      --help            print this help, then exit
      --version         print tar program version number, then exit
  -v, --verbose         verbosely list files processed
      --checkpoint      print directory names while reading the archive
      --totals          print total bytes written while creating archive
  -R, --block-number    show block number within archive with each message
  -w, --interactive     ask for confirmation for every action
      --confirmation    same as -w
 
Main operation mode:
  -t, --list              list the contents of an archive
  -x, --extract, --get    extract files from an archive
  -c, --create            create a new archive
  -d, --diff, --compare   find differences between archive and file system
  -r, --append            append files to the end of an archive
  -u, --update            only append files newer than copy in archive
  -A, --catenate          append tar files to an archive
      --concatenate       same as -A
      --delete            delete from the archive (not on mag tapes!)
 
Report bugs to <bug-tar@gnu.org>.
 
The backup suffix is `~', unless set with --suffix or SIMPLE_BACKUP_SUFFIX.
The version control may be set with --backup or VERSION_CONTROL, values are:

  t, numbered     make numbered backups
  nil, existing   numbered if numbered backups exist, simple otherwise
  never, simple   always make simple backups
 
Usage: %s [OPTION]...
  link to %s
  n [name]   Give a new file name for the next (and subsequent) volume(s)
 q          Abort tar
 !          Spawn a subshell
 ?          Print this list
 %s is not continued on this volume %s is the wrong size (%s != %s + %s) %s: Deleting %s
 %s: Not found in archive %s: Unknown file type; file ignored %s: Was unable to backup this file %s: illegal option -- %c
 %s: invalid option -- %c
 %s: option `%c%s' doesn't allow an argument
 %s: option `%s' is ambiguous
 %s: option `%s' requires an argument
 %s: option `--%s' doesn't allow an argument
 %s: option `-W %s' doesn't allow an argument
 %s: option `-W %s' is ambiguous
 %s: option requires an argument -- %c
 %s: unrecognized option `%c%s'
 %s: unrecognized option `--%s'
 ' --Continued at byte %s--
 --Mangled file names--
 --Volume Header--
 At beginning of tape, quitting now Attempting extraction of symbolic links as hard links Blanks in header where numeric %s value expected Cannot allocate buffer space Cannot combine --listed-incremental with --newer Cannot execute remote shell Cannot update compressed archives Cannot use compressed or remote archives Cannot use multi-volume compressed archives Cannot verify compressed archives Cannot verify multi-volume archives Cannot verify stdin/stdout archive Child died with signal %d Child returned status %d Conflicting archive format options Conflicting compression options Contents differ Could only read %lu of %lu bytes Cowardly refusing to create an empty archive Creating directory: Deleting non-header from archive Device number differs Device number out of range EOF where user reply was expected Error exit delayed from previous errors Extracting contiguous files as regular files File type differs GNU `tar' saves many files together into a single tape or disk archive, and
can restore individual files from the archive.
 GNU features wanted on incompatible archive format Garbage command Generate data files for GNU tar test suite.
 Gid differs If a long option shows an argument as mandatory, then it is mandatory
for the equivalent short option also.

  -l, --file-length=LENGTH   LENGTH of generated file
  -p, --pattern=PATTERN      PATTERN is `default' or `zeros'
      --help                 display this help and exit
      --version              output version information and exit
 Inode number out of range Invalid blocking factor Invalid mode given on option Invalid record size Invalid tape length Invalid value for record_size Missing file name after -C Mod time differs Mode differs More than one threshold date Multiple archive files requires `-M' option No archive name given No new volume; exiting.
 Not linked to %s Obsolete option name replaced by --absolute-names Obsolete option name replaced by --backup Obsolete option name replaced by --block-number Obsolete option name replaced by --blocking-factor Obsolete option name replaced by --read-full-records Obsolete option name replaced by --touch Obsolete option, now implied by --blocking-factor Old option `%c' requires an argument. Options `-%s' and `-%s' both want standard input Options `-[0-7][lmh]' not supported by *this* tar Premature end of file Prepare volume #%d for %s and hit return:  Read checkpoint %d Reading %s
 Record size = %lu blocks Record size must be a multiple of %d. Renamed %s to %s Seek direction out of range Seek offset out of range Size differs Skipping to next header Symlink differs Symlinked %s to %s This does not look like a tar archive This volume is out of sequence Too many errors, quitting Try `%s --help' for more information.
 Uid differs Unexpected EOF in archive Unexpected EOF in mangled names Unknown demangling command %s Unknown system error VERIFY FAILURE: %d invalid header(s) detected Valid arguments are: Verify  Visible long name error Visible longname error WARNING: Archive is incomplete WARNING: No volume header Write checkpoint %d You may not specify more than one `-Acdtrux' option You must specify one of the `-Acdtrux' options ` block %s:  block %s: ** Block of NULs **
 block %s: ** End of File **
 exec/tcp: Service not available rmtd: Cannot allocate buffer space
 rmtd: Garbage command %c
 rmtd: Premature eof
 stdin stdout tar (child) tar (grandchild) Project-Id-Version: tar 1.13.7
POT-Creation-Date: 2001-09-26 13:54-0700
PO-Revision-Date: 1999-08-23 09:31+08:00
Last-Translator: Const Kaplinsky <const@ce.cctpu.edu.ru>
Language-Team: Russian <ru@li.org>
MIME-Version: 1.0
Content-Type: text/plain; charset=koi8-r
Content-Transfer-Encoding: 8bit
 
������ � �������:
  -b, --blocking-factor=�����    ����� x 512 ���� ��� ����� ������
      --record-size=������       ������ ������ � ������, ������� 512
  -i, --ignore-zeros             ������������ ������ (EOF) �����
  -B, --read-full-records        �������� ����� � ����� ��� ������
                                  (��� ������� 4.2BSD)
 
����� ���������:
  -f, --file=�����               ������������ ���� ��� ���������� �����
      --force-local              ������� ����� ���������, ���� ���� ���� `:'
      --rsh-command=�������      ������������ ������� ��� ��������� ������
  -[0-7][lmh]                    ������� ���������� � ���������
  -M, --multi-volume             �������� � ������������ ��������
  -L, --tape-length=�����        ������� ����� ����� ������ ����� x 1024 ����
  -F, --info-script=����         ��������� ��������� ���� ����� ������ �����
      --new-volume-script=����   �� ��, ��� � -F ����
      --volno-file=����          ������/������ ����� ���� ��/� ����(�)
 
���� ��� �������� ����� ����� ��������, �� �� ����� ����� � ���
�������������� ��������� �����. ���������� � ��������������� �����������.
 
����� ����������:
      --help            ������� ��� ��������� � �����
      --version         ������� ������ ���� ��������� � �����
  -v, --verbose         �������� ����� �������������� ������
      --checkpoint      �������� ����� ��������� ��� ������ �� ������
      --totals          ������� ����� ���������� ���������� ����
  -R, --block-number    �������� ����� ����� ������ � ������ ���������
  -w, --interactive     ����������� ������������� ��� ������ ��������
      --confirmation    �� ��, ��� � -w
 
�������� ��������:
  -t, --list              ������� ������ ����������� ������
  -x, --extract, --get    ������� ����� �� ������
  -c, --create            ������� ����� �����
  -d, --diff, --compare   ����� �������� ����� ������� � �������� ��������
  -r, --append            ������������ ����� � ����� ������
  -u, --update            ������������ ������ �� �����, ������� �����, ���
                           �� ����� � ������
  -A, --catenate          ������������ tar-����� � ������
      --concatenate       �� ��, ��� � -A
      --delete            ������� �� ������ (�� �� ��������� ������!)
 
�� ������� ��������� �� ������ <bug-tar@gnu.org>.
 
����� ��������� ����� ������������� �������� `~', ���� � ������� --suffix
��� SIMPLE_BACKUP_SUFFIX �� ���������� ���� �������.
���������� �������� ����� ���������� � ������� --backup ��� VERSION_CONTROL,
��� ���� �������� ��������� ��������:

  t, numbered     ������ ������������ ��������� �����
  nil, existing   ������������, ���� ������� ��� ����, ����� �������
  never, simple   ������ ������ ������� ��������� �����
 
�������������: %s [�����]...
  ������ �� %s
  n [name]   ���� ����� ��� ��� ���������� ���� (� ����������� �����)
 q          �������� ���������� tar
 !          ������� ��������� ���������
 ?          ���������� ���� ������
 %s �� ������������ �� ���� ���� � %s �������� ������ (%s != %s + %s) %s: �������� %s
 %s: � ������ �� ������ %s: ����������� ��� �����; ������������ %s: �� ������� ������� ��������� ����� ������� ����� %s: ������������ ���� -- %c
 %s: �������� ���� -- %c
 %s: ���� `%c%s' �� ��������� �������� ���������
 %s: ������������� ���� `%s'
 %s: ���� `%s' ������� �������� ���������
 %s: ���� `--%s' �� ��������� �������� ���������
 %s: ���� `-W %s' �� ��������� �������� ���������
 %s: ������������ ���� `-W %s'
 %s: ���� ������� �������� ��������� -- %c
 %s: ����������� ���� `%c%s'
 %s: ����������� ���� `--%s'
 ' --����������� � ������� %s--
 --�������� ����� ������--
 --��������� ����--
 ������� ������� -- ������ �����, ����������� ������ ������� ������� ���������� ������ ��� ������� � ��������� ������ ���� ������ ���������� ��������� �������� %s ���������� �������� ����� ��� ������ ����� --listed-incremental � --newer ������ ��������� ������ ���������� ��������� ��������� ��������� ��������� ���������� ������ ������� ���������� ������������� ������ ��� ��������� ������� ���������� ������������� ������ ����������� ������� ���������� �������� ������ ������� ���������� �������� ����������� ������� ���������� �������� ������ � ����������� �����/������ ���������� ���������� ��������-������� ��������� �������� %d �������-������� ��������� ������ %d ������������� �����, ����������� ������ ������ ������������� �����, ����������� ��� ������ ���������� ����������� ������� ��������� ������ %lu �� %lu ���� ������ ����� �� �������� ������� ������ �������� ��������: �������� "�����������" �� ������ ������ ��������� ����������� ����� ���������� ��� ��������� ����� ����� ��� �������� ������ ������������ �����, ���������� �� ����������� ���������� ������ ���������� ����������� ������ ��� ������� ���� ������ ����������� GNU `tar' ��������� ��������� ������ � ����� ������ �� ����� ��� �����
� ����� ������������� ��������� ����� �� ������.
 ���������� GNU ��������� �� ������������� ������� ������ �������� ������� ������������ ������ ������ ��� ������������ GNU tar.
 Gid ����������� ���� ��� �������� ����� ����� ��������, �� �� ����� ����� � ���
�������������� ��������� �����. ���������� � ��������������� �����������.

   -l, --file-length=�����    ����� ������������ �����
   -p, --pattern=������       ������ ������ ���� `default' ��� `zeros'
       --help                 ������� ��� ��������� � �����
       --version              ������� ���������� � ������ � �����
 ����� ���� Inode ��� ��������� �������� ������ ����� ������ ������ �������� ����� ������� �������� ������ ������ �������� ����� ����� ������������ �������� ��� record_size ��������� ��� ����� ����� -C ������� ����������� ����������� ����� ������� ����������� ������ ����� ��������� ���� �������� ���������� ������� ������� ����� `-M' �� ������� ��� ������ ��� ������ ����, ����������� ������.
 �� ��������� �� %s ���������� ��� ����� �������� �� --absolute-names ���������� ��� ����� �������� �� --backup ���������� ��� ����� �������� �� --block-number ���������� ��� ����� �������� �� --blocking-factor ���������� ��� ����� �������� �� --read-full-records ���������� ��� ����� �������� �� --touch ���������� ����, ������ ����������� � ������� --blocking-factor ������ ���� `%c' ������� �������� ���������. ��� ����� `-%s' � `-%s' ���������� ����������� ���� ������ ������ ��������� tar �� ������������ ����� `-[0-7][lmh]' ��������������� ����� ����� ����������� ��� ����� %d ��� %s � ������� ������� �����:  ����������� ����� ������ %d ������ %s
 ������ ������ (� ������) = %lu ������ ������ ������ ���� ������ %d. %s ������������ � %s ����������� ���������������� ��� ��������� �������� ���������������� ��� ��������� ������� ����������� ������� �� ���������� ��������� ���������� ������ ����������� ������� ���������� ������ %s �� %s ��� �� ������ �� tar-����� ���� ��� �������� ������������������ ������� ����� ������, ����������� ������ ���������� `%s --help' ��� ��������� ����� ��������� ����������.
 Uid ����������� �������������� ����� ����� � ������ �������������� ����� ����� � �������� ������ ����������� ������� %s �������������� �������� ���� ����������� ��������� ������ ������ ��������: ���������� �������� ����������: %d ���������� ���������: ��������  ������ ��-�� �������� ����� ������ ��-�� �������� ����� ��������������: ����� �� ������ ��������������: ��� ��������� ���� ����������� ����� ������ %d �� �� ������ ������� ����� ������ ����� `-Acdtrux' �� ������ ������� ���� �� ������ `-Acdtrux' ` ���� %s:  ���� %s: ** ���� ����� **
 ���� %s: ** ����� ����� **
 exec/tcp: ������ ���������� rmtd: ���������� �������� ����� ��� ������
 rmtd: �������� ������� %c
 rmtd: ��������������� ����� �����
 ����������� ���� ����������� ����� tar (�������) tar (������� �������) 