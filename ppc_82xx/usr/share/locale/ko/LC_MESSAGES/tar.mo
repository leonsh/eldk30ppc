��       W     �     �      �  -  �  �  �     @  -  A     o     �   �  �   "  -     P     a   #  z   "  �     �   "  �   5  �     -     J   !  f   (  �   +  �   !  �   #  �   "  #     F   "  _     �     �      �   !  �   ,  �     &   ,  6     c  Y  o     �     �               0   +  =     i          �   1  �   )  �   /     2  5   4  h   (  �   1  �   %  �   0     1  O     �     �     �   %  �     �     �     �          "   %  5     [     z   &  �     �     �     �   -  �     $     ,     D     [     z     �   3  �   .  �        #  +     O     i     ~     �     �     �  $  �  Z  �  �  (       .  ")     $X     $p   �  $|   %  %      %F     %U   '  %i   "  %�     %�   "  %�   )  %�     &     &/   $  &L   4  &q   .  &�   $  &�   '  &�   )  '"   "  'L     'o     '�     '�   !  '�   &  '�   '  '�     (%   .  (3     (b  7  (q     )�     )�     )�     )�     *   +  *!   #  *M     *q     *�   1  *�   +  *�   /  *�   /  +)   4  +Y   (  +�   -  +�   %  +�   0  ,   1  ,<     ,n     ,}     ,�   (  ,�     ,�     ,�     ,�     ,�     -   '  -.     -V     -t   ,  -�     -�     -�     -�   (  .      .)     ./     .E     .Z     .x     .�   1  .�   +  .�     .�   %  /     />     /V     /h     /q   
  /z   
  /�      >   L       "                  I   4   B   /   N                      )       @       T           U                     F       Q      =   K   %               '         R          <   M           W   8   O   3       ;       :             G      0      !       5                       J      +   S   	   ,   2      (              $   P          D   7   
      V                        H   .          C   &          -   #   1      A         *         9       6   E          ? 
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
Usage: %s [OPTION]...
  link to %s
  n [name]   Give a new file name for the next (and subsequent) volume(s)
 q          Abort tar
 !          Spawn a subshell
 ?          Print this list
 %s is not continued on this volume %s: Deleting %s
 %s: Not found in archive %s: Unknown file type; file ignored %s: Was unable to backup this file --Volume Header--
 At beginning of tape, quitting now Attempting extraction of symbolic links as hard links Cannot allocate buffer space Cannot execute remote shell Cannot update compressed archives Cannot use compressed or remote archives Cannot use multi-volume compressed archives Cannot verify compressed archives Cannot verify multi-volume archives Cannot verify stdin/stdout archive Child returned status %d Conflicting archive format options Conflicting compression options Creating directory: Deleting non-header from archive EOF where user reply was expected Extracting contiguous files as regular files Garbage command Generate data files for GNU tar test suite.
 Gid differs If a long option shows an argument as mandatory, then it is mandatory
for the equivalent short option also.

  -l, --file-length=LENGTH   LENGTH of generated file
  -p, --pattern=PATTERN      PATTERN is `default' or `zeros'
      --help                 display this help and exit
      --version              output version information and exit
 Invalid mode given on option Invalid value for record_size Missing file name after -C Mod time differs Mode differs Multiple archive files requires `-M' option No archive name given No new volume; exiting.
 Not linked to %s Obsolete option name replaced by --absolute-names Obsolete option name replaced by --backup Obsolete option name replaced by --block-number Obsolete option name replaced by --blocking-factor Obsolete option name replaced by --read-full-records Obsolete option name replaced by --touch Obsolete option, now implied by --blocking-factor Old option `%c' requires an argument. Options `-%s' and `-%s' both want standard input Options `-[0-7][lmh]' not supported by *this* tar Premature end of file Read checkpoint %d Reading %s
 Record size must be a multiple of %d. Renamed %s to %s Size differs Skipping to next header Symlink differs Symlinked %s to %s This does not look like a tar archive This volume is out of sequence Too many errors, quitting Try `%s --help' for more information.
 Uid differs Unexpected EOF in archive Unknown system error VERIFY FAILURE: %d invalid header(s) detected Verify  Visible long name error Visible longname error WARNING: Archive is incomplete WARNING: No volume header Write checkpoint %d You may not specify more than one `-Acdtrux' option You must specify one of the `-Acdtrux' options exec/tcp: Service not available rmtd: Cannot allocate buffer space
 rmtd: Garbage command %c
 rmtd: Premature eof
 stdin stdout tar (child) tar (grandchild) Project-Id-Version: GNU tar 1.12
POT-Creation-Date: 2001-09-26 13:54-0700
PO-Revision-Date: 2001-09-16 07:45+0200
Last-Translator: Bang Jun-Young <bangjy@nownuri.net>
Language-Team: Korean <ko@li.org>
MIME-Version: 1.0
Content-Type: text/plain; charset=EUC-KR
Content-Transfer-Encoding: 8bit
 
��ġ ���� ����:
  -b, --blocking-factor=BLOCK    ���ڵ�� BLOCK x 512 ����Ʈ
      --record-size=SIZE         ���ڵ�� SIZE ����Ʈ, 512�� ���
  -i, --ignore-zeros             ��ī�̺꿡�� ������ �� ������ �����մϴ�
                                 (EOF�� �ǹ���)
  -B, --read-full-records        ���� ���� �����ȭ�մϴ� (4.2BSD ������������)
 
��ġ ���ð� ��ȯ:
  -f, --file=ARCHIVE             ��ī�̺� ���� �Ǵ� ARCHIVE ��ġ�� ����մϴ�
      --force-local              �̸��� �ݷ��� �ִ� ��ī�̺� ���ϵ� ���� ���Ϸ�
                                 �ν��մϴ�
      --rsh-command=COMMAND      rsh ��� ���� COMMAND�� ����մϴ�
  -[0-7][lmh]                    ����̺�� ��� �е��� �����մϴ�
  -M, --multi-volume             ���� ���� ��ī�̺긦 ����/���/�����մϴ�
  -L, --tape-length=NUM          NUM x 1024 ����Ʈ�� �� �ڿ� �������� �ٲߴϴ�
  -F, --info-script=FILE         �� �������� ������ ��ũ��Ʈ�� �����մϴ�
                                 (-M�� ������)
      --new-volume-script=FILE   -F FILE�� ����
      --volno-file=FILE          FILE �ȿ� �ִ� ���� ��ȣ�� ���/�����մϴ�
 
���� ��¿� ���� �ɼ�:
      --help            �� ������ �μ��ϰ� �����ϴ�
      --version         tar ���α׷��� ���� ��ȣ�� �μ��ϰ� �����ϴ�
  -v, --verbose         ó���Ǵ� ������ ������� ����մϴ�
      --checkpoint      ��ī�̺긦 ���� ���� ���丮 �̸��� �μ��մϴ�
      --totals          ��ī�̺긦 ���� ���� ������ �� ����Ʈ ���� �μ��մϴ�
  -R, --block-number    �� �޽������� ��ī�̺곻�� ���� ��ȣ�� ǥ���մϴ�
  -w, --interactive     ��� �ൿ�� ���� Ȯ���� �䱸�մϴ�
      --confirmation    -w�� ����
 
�ֿ� ���� ���:
  -t, --list              ��ī�̺��� ���빰�� ����մϴ�
  -x, --extract, --get    ��ī�̺꿡�� ������ �����մϴ�
  -c, --create            ���ο� ��ī�̺긦 ����ϴ�
  -d, --diff, --compare   ��ī�̺�� ���� �ý��۰��� �������� ���մϴ�
  -r, --append            ��ī�̺� ���� ������ �߰��մϴ�
  -u, --update            ��ī�̺� ���� �ͺ��� ���ο� ���ϸ� �߰��մϴ�
  -A, --catenate          ��ī�̺꿡 tar ������ �߰��մϴ�
      --concatenate       -A�� ����
      --delete            ��ī�̺�κ��� �����մϴ� (�ڱ� ���������� �ȵ�!)
 
����: %s [�ɼ�]...

  %s�� ��ũ
  n [�̸�]   ����(�� �� ������) ������ ���� �� ���� �̸��� �����մϴ�
 q          tar�� �ߴ��մϴ�
 !          ������� �����մϴ�
 ?          �� ����� �μ��մϴ�
 %s�� �� ������ ���ӵǾ� ���� �ʽ��ϴ� %s: %s�� ����
 %s: ��ī�̺꿡 ���� %s: �� �� ���� ���� Ÿ��; ������ ���õ� %s: �� ������ ����� �� �������ϴ� --���� ���--
 �������� ���� �κп��� ���� ������ ��ȣ ��ũ�� �ϵ� ��ũ�� �����ϰ� �ֽ��ϴ� ���� ������ �Ҵ��� �� �����ϴ� ���� ���� ������ �� �����ϴ� ����� ��ī�̺긦 ������ �� �����ϴ� ����� ��ī�̺곪 ���� ��ī�̺긦 ����� �� �����ϴ� ����� ����-���� ��ī�̺긦 ����� �� �����ϴ� ����� ��ī�̺긦 ������ �� �����ϴ� ����-���� ��ī�̺긦 ������ �� �����ϴ� ǥ����/��� ��ī�̺긦 ������ �� �����ϴ� �ڽ��� ���� %d�� �ǵ��� �־����ϴ� �򰥸��� ��ī�̺� ���� �ɼ� �򰥸��� ���� �ɼ� ���丮�� ����� ��: ��ī�̺꿡�� ����� �κ��� ������ ������� ������ �ʿ��� ���� EOF�� ���� ���ӵǾ� �ִ� ������ �Ϲ� ���Ϸ� ������ ������� ���� GNU tar ���� ������ ������ ������ �����մϴ�.
 gid�� �ٸ��ϴ� �� �ɼǿ� �ΰ��Ǵ� �μ��� ���� ��, �̴� ������ �ǹ��� ª�� �ɼǿ���
����˴ϴ�.

  -l, --file-length=����     �����Ǵ� ������ ����
  -p, --pattern=����         ������ `default'�� `zeros'�Դϴ�
      --help                 �� ������ �����ְ� ��Ĩ�ϴ�
      --version              ���� ������ ����ϰ� ��Ĩ�ϴ�
 �ɼǿ� �������� ��尡 �־��� record_size�� �������� �� -C �ڿ� ���� �̸��� ������ ���� �ð��� �ٸ��ϴ� ��尡 �ٸ��ϴ� ���� ��ī�̺� ������ `-M' �ɼ��� �ʿ��մϴ� ��ī�̺� �̸��� �־����� �ʾҽ��ϴ� �� ������ �ƴ�; ����.
 %s�� ������� ���� --absolute-names�� ��ü�Ǿ� ������� �� �ɼ� �̸� --backup���� ��ü�Ǿ� ������� �� �ɼ� �̸� --block-number�� ��ü�Ǿ� ������� �� �ɼ� �̸� --block-factor�� ��ü�Ǿ� ������� �� �ɼ� �̸� --read-full-records�� ��ü�Ǿ� ������� �� �ɼ� �̸� --touch�� ��ü�Ǿ� ������� �� �ɼ� �̸� --blocking-factor�� ���ԵǾ� ������� �� �ɼ� ������ �ɼ� `%c'�� �μ��� �ʿ��մϴ�. `-%s'�� `-%s' �ɼ��� ��� ǥ�� �Է��� �ʿ��մϴ� `-[0-7][lmh]' �ɼ��� �� tar���� �������� �ʽ��ϴ� �߸��� ���� �� �˻����� %d�� ���� %s�� �д� ��
 ���ڵ� ũ��� %d�� ����� �Ǿ�� �մϴ�. %s�� %s�� �̸� �ٲ� ũ�Ⱑ �ٸ��ϴ� ���� ����� �ǳ� �� ��ȣ��ũ�� �ٸ��ϴ� %s���� %s�� ��ȣ��ũ�Ǿ��� �̰��� tar ��ī�̺�ó�� ������ �ʽ��ϴ� �� ������ ������ ������ϴ� ������ �ʹ� ���Ƽ� �����մϴ� �� ���� ������ ������ `%s --help' �Ͻʽÿ�.
 uid�� �ٸ��ϴ� ��ī�̺꿡 ����ġ ���� EOF �� �� ���� �ý��� ���� ���� ����: %d���� �������� ����� ����� ����  �������� �� �̸� ���� �������� ���̸� ���� ���: ��ī�̺갡 �ҿ����մϴ� ���: ���� ��� ���� �˻����� %d�� �� `-Acdtrux' �ɼ� �� �ϳ� �̻��� �����ϸ� �� �˴ϴ� `-Acdtrux' �ɼǵ� �� �ϳ��� �����ؾ� �մϴ� exec/tcp: �� �� ���� ���� rmtd: ���� ������ �Ҵ��� �� �����ϴ�
 rmtd: ������� ���� %c
 rmtd: �߸��� eof
 ǥ���Է� ǥ����� tar (�ڽ�) tar (����) 