��       b     ,   �  <      H  -  I  �  	w   �        �  -  �  7  �     �        �     "  �     �     �   #     "  '     J     b   "  u   5  �     �     �   !     (  )   +  R   !  ~   #  �   "  �     �   "        #   ,  C     p      �   !  �   '  �   ,  �   {     2  �     �   ,  �       Y       n     �     �     �     �     �   +  �     +     A     Z   1  k   )  �   /  �   2  �   4  *   (  _   1  �   %  �   0  �   +     1  =     o     �     �   %  �     �     �     �              %  #     I     h   &  �     �     �     �     �        -  "     P     X     p     �     �     �   3  �   .       7   #  W     {     �     �     �     �     �  B  �  B     �  !Z   �  #�  ;  $�  F  &�  O  )&     *v     *�   �  *�   $  +3     +X   !  +j   +  +�   )  +�     +�     +�     ,   /  ,+   $  ,[   $  ,�   &  ,�   7  ,�   8  -   &  -=   ,  -d   +  -�     -�     -�     -�   &  .     .4   !  .G   #  .i   ,  .�   3  .�   s  .�   A  /b     /�   )  /�     /�  y  /�   $  1k     1�   $  1�     1�     1�     1�   +  2	     25     2R     2q   9  2�   1  2�   7  2�   :  3(   <  3c   0  3�   2  3�   &  4   /  4+   *  4[   0  4�     4�     4�     4�   &  4�     5     50     5?     5Y     5i     5z     5�      5�   1  5�     6     6   "  6-   $  6P     6u   -  6�     6�      6�     6�   !  7
   #  7,     7P   ;  7f   6  7�      7�   +  7�      8&     8G     8[     8a     8h     8t                U          J   /                               T   :   M      ]   H      N   3       4      R   Y              V   $   2   [   -   L   *   D                  )   #   
   (   E   K   S              a   C                  \       9   6   @      8   ?   ,   !                       O      "   <   B                  G   +      ;   b   I   >   5           1   P          W   ^      Q      Z   	              0   7   &   `       A   .         '   _       F       X   %               = 
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
 %s is not continued on this volume %s: Deleting %s
 %s: Not found in archive %s: Unknown file type; file ignored %s: Was unable to backup this file --Mangled file names--
 --Volume Header--
 At beginning of tape, quitting now Attempting extraction of symbolic links as hard links Cannot allocate buffer space Cannot execute remote shell Cannot update compressed archives Cannot use compressed or remote archives Cannot use multi-volume compressed archives Cannot verify compressed archives Cannot verify multi-volume archives Cannot verify stdin/stdout archive Child returned status %d Conflicting archive format options Conflicting compression options Cowardly refusing to create an empty archive Creating directory: Deleting non-header from archive EOF where user reply was expected Error exit delayed from previous errors Extracting contiguous files as regular files GNU `tar' saves many files together into a single tape or disk archive, and
can restore individual files from the archive.
 GNU features wanted on incompatible archive format Garbage command Generate data files for GNU tar test suite.
 Gid differs If a long option shows an argument as mandatory, then it is mandatory
for the equivalent short option also.

  -l, --file-length=LENGTH   LENGTH of generated file
  -p, --pattern=PATTERN      PATTERN is `default' or `zeros'
      --help                 display this help and exit
      --version              output version information and exit
 Invalid mode given on option Invalid value for record_size Missing file name after -C Mod time differs Mode differs More than one threshold date Multiple archive files requires `-M' option No archive name given No new volume; exiting.
 Not linked to %s Obsolete option name replaced by --absolute-names Obsolete option name replaced by --backup Obsolete option name replaced by --block-number Obsolete option name replaced by --blocking-factor Obsolete option name replaced by --read-full-records Obsolete option name replaced by --touch Obsolete option, now implied by --blocking-factor Old option `%c' requires an argument. Options `-%s' and `-%s' both want standard input Options `-Aru' are incompatible with `-f -' Options `-[0-7][lmh]' not supported by *this* tar Premature end of file Read checkpoint %d Reading %s
 Record size must be a multiple of %d. Renamed %s to %s Size differs Skipping to next header Symlink differs Symlinked %s to %s This does not look like a tar archive This volume is out of sequence Too many errors, quitting Try `%s --help' for more information.
 Uid differs Unexpected EOF in archive Unexpected EOF in mangled names Unknown demangling command %s Unknown system error VERIFY FAILURE: %d invalid header(s) detected Verify  Visible long name error Visible longname error WARNING: Archive is incomplete WARNING: No volume header Write checkpoint %d You may not specify more than one `-Acdtrux' option You must specify one of the `-Acdtrux' options exec/tcp: Service not available rmtd: Cannot allocate buffer space
 rmtd: Garbage command %c
 rmtd: Premature eof
 stdin stdout tar (child) tar (grandchild) Project-Id-Version: tar 1.13.12
POT-Creation-Date: 2001-09-26 13:54-0700
PO-Revision-Date: 2001-05-10 14:18GMT+0700
Last-Translator: Tedi Heriyanto <tedi_h@gmx.net>
Language-Team: Indonesian <id@li.org>
MIME-Version: 1.0
Content-Type: text/plain; charset=ISO-8859-1
Content-Transfer-Encoding: 8bit
X-Generator: KBabel 0.8
 
Device blok:
  -b, --blocking-factor=BLOCKS   BLOCKS x 512 bytes setiap record
      --record-size=SIZE         SIZE bytes setiap record, kelipatan dari 512
  -i, --ignore-zeros             abaikan zeroed blocks dalam archive (berarti EOF)
  -B, --read-full-records        block ulang pada saat baca (untuk 4.2BSD pipes)
 
Pemilihan dan penggantian device:
  -f, --file=ARCHIVE             gunakan file atau device ARCHIVE
      --force-local              file archive local, walaupun memiliki colon
      --rsh-command=COMMAND      gunakan COMMAND remote selain rsh
  -[0-7][lmh]                    menentukan drive dan density
  -M, --multi-volume             buat/lihat/extract archive multi-volume
  -L, --tape-length=NUM          ganti tape setelah menulis NUM x 1024 bytes
  -F, --info-script=FILE         jalankan script pada akhir setiap tape (berlaku untuk -M)
      --new-volume-script=FILE   sama seperti -F FILE
      --volno-file=FILE          gunakan/update volume number dalam FILE
 
Bila long option menunjukkan argumen sebagai mandatory, maka opsi tersebut
adalah mandatory juga untuk short option. Hal yang sama berlaku untuk
optional argumen.
 
Output informasi:
      --help            menampilkan help ini dan keluar
      --version         menampilkan versi program tar dan keluar
  -v, --verbose         menampilkan keterangan file yang sedang diproses
      --checkpoint      menampilkan nama directory names saat membaca archive
      --totals          menampilkan jumlah byte yang ditulis saat membuat archive
  -R, --block-number    menampilkan nomor blok dalam archive dengan setiap pesan
  -w, --interactive     interaktif, meminta konfirmasi untuk setiap tindakan
      --confirmation    sama seperti -w
 
Modus operasi utama:
  -t, --list              melihat isi archive
  -x, --extract, --get    mengekstrak file archive
  -c, --create            membuat archive baru
  -d, --diff, --compare   mencari beda antara archive dan file system
  -r, --append            menambahkan file pada bagian akhir archive
  -u, --update            hanya menambahkan file yang lebih baru daripada yang ada di archive
  -A, --catenate          append file tar ke dalam archive
      --concatenate       sama seperti -A
      --delete            menghapus dari archive (tidak berlaku untuk mag tapes!)
 
Akhiran dari backup adalah `~', kecuali bila diset dengan --suffix atau SIMPLE_BACKUP_SUFFIX.
Version control dapat diset dengan -backup atau VERSION_CONTROL, nilainya:

  t, numbered     gunakan numbered backups
  nil, existing   beri nomor bila nomor backup telah ada, simple sebaliknya
  never, simple   selalu buat simple backups
 
Gunakan: %s [OPTION]...
  link ke %s
  n [name]   Memberi nama baru pada file untuk volume selanjutnya
 q          Batalkan tar
 !          Spawn subshell
 ?          Cetak keterangn ini
 %s tidak dilanjutkan pada volume ini %s: Menghapus %s
 %s: Tidak ditemukan dalam archive %s: Tipe file tidak dikenal; file diabaikan %s: Tidak dapat melakukan backup file ini --Mangled file names--
 --Volume Header--
 Berada pada awal tape, keluar Mencoba extract symbolic link sebagai hard link Tidak dapat mengalokasi buffer space Tidak dapat menjalankan remote shell Tidak dapat update compressed archives Tidak dapat menggunakan compressed atau remote archives Tidak dapat menggunakan multi-volume compressed archives Tidak dapat verify compressed archives Tidak dapat verifikasi multi-volume archives Tidak dapat verifikasi stdin/stdout archive Child mengembalikan status %d Opsi format archive konflik Opsi kompresi konflik Tidak bisa membuat archive yang kosong Membuat direktori: Menghapus non-header dari archive EOF pada saat user reply diharapkan Kesalahan exit ditunda dari error sebelumnya Sedang extract contiguous file sebagai regular file GNU `tar' menyimpan sejumlah file dalam sebuah tape atau disk archive, dan
dapat restore sebuah file dari archive.
 Feature GNU dibutuhkan untuk format archive yang tidak kompatibel Command tidak terpakai Hasilkan file data untuk GNU test suite.
 Gid berbeda Bila long option menunjukkan argumen sebagai mandatory, maka opsi tersebut
adalah mandatory juga untuk short option. 

  -l, --file-length=LENGTH   LENGTH dari file yang dihasilkan
  -p, --pattern=PATTERN      PATTERN adalah `default' atau `zeros'
      --help                 menampilkan help ini dan keluar
      --version              menampilkan informasi versi dan keluar
 Mode tidak tepat diberikan pada opsi Nilai record_size salah File name tidak ditemukan setelah -C Mod time berbeda Mode berbeda Lebih dari satu treshold date File multiple archive membutuhkan opsi '-M' Tidak diberikan nama archive Tidak ada new volume; keluar.
 Tidak dilink ke %s Opsi tidak berlaku lagi, digantikan oleh --absolute-names Opsi tidak berlaku lagi, digantikan oleh --backup Opsi tidak berlaku lagi, digantikan oleh --block-number Opsi tidak berlaku lagi, digantikan oleh --blocking-factor Opsi tidak berlaku lagi, digantikan oleh --read-full-records Opsi tidak berlaku lagi, digantikan oleh --touch Opsi tidak berlaku lagi, gunakan --blocking-factor Opsi 'lama' `%c' membutuhkan argument. Opsi `-%s' dan `-%s' membutuhkan standard input Opsi '-Aru' tidak kompatibel dengan `-f -' Opsi `-[0-7][lmh]' tidak didukung oleh tar *ini* Akhir file prematur Membaca checkpoint %d Membaca %s
 Jumlah record harus kelipatan dari %d. Berganti nama dari %s ke %s Ukuran berbeda Skip ke header berikutnya Symlink berbeda Symlink %s ke %s Sepertinya bukan tar archive Volume ini di luar urutan Terlalu banyak kesalahan, keluar Ketik `%s --help' untuk informasi lebih lengkap.
 Uid berbeda Unexpected EOF dalam archive Unexpected EOF dalam mangled names Perintah demangling %s tidak dikenal Kesalahan sistem tidak dikenal GAGAL VERIFIKASI: ditemukan %d invalid header Verifikasi  Kesalahan visible long file name Kesalahan visible longname PERINGATAN: Archive tidak lengkap PERINGATAN: Tidak ada volume header Menulis checkpoint %d Anda tidak bisa menjalankan lebih dari satu opsi `-Acdtrux' Anda harus menggunakan salah satu dari opsi `-Acdtrux' exec/tcp: Service tidak tersedia rmtd: Tidak dapat mengalokasi buffer space
 rmtd: Command tidak terpakai %c
 rmtd: EOF prematur
 stdin stdout tar (child) tar (grandchild) 