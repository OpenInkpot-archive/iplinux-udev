VOLUME_ID_OBJS=						\
	$(VOLUME_ID_BASE)/ext/ext.o			\
	$(VOLUME_ID_BASE)/fat/fat.o			\
	$(VOLUME_ID_BASE)/hfs/hfs.o			\
	$(VOLUME_ID_BASE)/highpoint/highpoint.o		\
	$(VOLUME_ID_BASE)/iso9660/iso9660.o		\
	$(VOLUME_ID_BASE)/jfs/jfs.o			\
	$(VOLUME_ID_BASE)/linux_raid/linux_raid.o	\
	$(VOLUME_ID_BASE)/linux_swap/linux_swap.o	\
	$(VOLUME_ID_BASE)/lvm/lvm.o			\
	$(VOLUME_ID_BASE)/mac/mac.o			\
	$(VOLUME_ID_BASE)/msdos/msdos.o			\
	$(VOLUME_ID_BASE)/ntfs/ntfs.o			\
	$(VOLUME_ID_BASE)/reiserfs/reiserfs.o		\
	$(VOLUME_ID_BASE)/udf/udf.o			\
	$(VOLUME_ID_BASE)/ufs/ufs.o			\
	$(VOLUME_ID_BASE)/xfs/xfs.o			\
	$(VOLUME_ID_BASE)/cramfs/cramfs.o		\
	$(VOLUME_ID_BASE)/hpfs/hpfs.o			\
	$(VOLUME_ID_BASE)/romfs/romfs.o			\
	$(VOLUME_ID_BASE)/sysv/sysv.o			\
	$(VOLUME_ID_BASE)/dasd/dasd.o			\
	$(VOLUME_ID_BASE)/volume_id.o			\
	$(VOLUME_ID_BASE)/util.o

VOLUME_ID_HEADERS=					\
	$(VOLUME_ID_BASE)/ext/ext.h			\
	$(VOLUME_ID_BASE)/fat/fat.h			\
	$(VOLUME_ID_BASE)/hfs/hfs.h			\
	$(VOLUME_ID_BASE)/highpoint/highpoint.h		\
	$(VOLUME_ID_BASE)/iso9660/iso9660.h		\
	$(VOLUME_ID_BASE)/jfs/jfs.h			\
	$(VOLUME_ID_BASE)/linux_raid/linux_raid.h	\
	$(VOLUME_ID_BASE)/linux_swap/linux_swap.h	\
	$(VOLUME_ID_BASE)/lvm/lvm.h			\
	$(VOLUME_ID_BASE)/mac/mac.h			\
	$(VOLUME_ID_BASE)/msdos/msdos.h			\
	$(VOLUME_ID_BASE)/ntfs/ntfs.h			\
	$(VOLUME_ID_BASE)/reiserfs/reiserfs.h		\
	$(VOLUME_ID_BASE)/udf/udf.h			\
	$(VOLUME_ID_BASE)/ufs/ufs.h			\
	$(VOLUME_ID_BASE)/xfs/xfs.h			\
	$(VOLUME_ID_BASE)/cramfs/cramfs.h		\
	$(VOLUME_ID_BASE)/sysv/sysv.h			\
	$(VOLUME_ID_BASE)/romfs/romfs.h			\
	$(VOLUME_ID_BASE)/dasd/dasd.h			\
	$(VOLUME_ID_BASE)/volume_id.h			\
	$(VOLUME_ID_BASE)/util.h