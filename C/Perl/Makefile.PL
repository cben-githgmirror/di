use 5.00503;
use ExtUtils::MakeMaker;
# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.
WriteMakefile(
    NAME              => 'Filesys::di',
    VERSION_FROM      => 'lib/Filesys/di.pm', # finds $VERSION
    PREREQ_PM         => {'Test'        =>  0, }, # e.g., Module::Name => 1.1
    ($] >= 5.005 ?     ## Add these new keywords supported since 5.005
      (ABSTRACT_FROM  => 'lib/Filesys/di.pm', # retrieve abstract from module
       AUTHOR         => 'Brad Lanam <brad.lanam.di@gmail.com>') : ()),
    LIBS              => ['-L.. -ldiperl'], # e.g., '-lm'
    DEFINE            => '', # e.g., '-DHAVE_SOMETHING'
    INC               => '-I.. -I.', # e.g., '-I. -I/usr/include/other'
	# Un-comment this if you add C files to link with later:
    # OBJECT            => '$(O_FILES)', # link all the C files too
);
