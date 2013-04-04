
CLEANFILES			= *~
DISTCLEANFILES			= wicked.pc
MAINTAINERCLEANFILES		= Makefile.in aclocal.m4 config.guess config.*   \
				  configure depcomp install-sh ltmain.sh missing \
				  $(DIST_ARCHIVES) $(PACKAGE)-*.tar.bz2

SUBDIRS				= include src util schema client server nanny \
				  autoip4 dhcp4 dhcp6 etc extensions man testing


pkgconfig_DATA			= wicked.pc

wicked_doc_data			= README TODO COPYING ANNOUNCE		\
				  $(wildcard ChangeLog ChangeLog.git)	\
				  Documentation/WhyWicked.txt
#wicked_doc_DATA		= $(wicked_doc_data)

EXTRA_DIST			= VERSION wicked.pc.in wicked-rpmlintrc	\
				  wicked.spec.in wicked.spec samples	\
				  $(wicked_doc_data)


# Build test RPMs, no checks
rpmbuild: dist-bzip2
	rpmbuild -ta $(distdir).tar.bz2

# Build source package with all checks enabled (unlike 'make dist')
package: all check distcheck show-commit-state

dist-hook: VERSION $(distdir)/wicked.spec $(distdir)/ChangeLog.git

$(distdir)/wicked.spec: wicked.spec
	@cp $< $@

$(distdir)/ChangeLog.git: check-git-tool
	@if  [ -d $(top_srcdir)/.git ]; then \
	  LANG=C git log --decorate=short -- $(top_srcdir) > $(distdir)/ChangeLog.git ; \
	fi

show-commit-state: check-git-tool
	@if  [ -d $(top_srcdir)/.git ]; then \
	  cd $(top_srcdir) && \
	  LC_ALL=C git status 2>&1 | \
	  grep -qsE 'Untracked|to be committed|not staged for commit' && { \
	    echo -e "##\n## *** NOTE: There are uncommited changes:\n##" ; \
	    git status 2>&1 || : ; \
	  } || : ; \
	fi

check-git-tool:
	@if ! git --version &>/dev/null ; then \
	  echo 'ERROR: Unable to find the '\'git\'' version control client!'; \
	  echo '       Please install git (git-core) and try again.'        ; \
	  exit 1 ; \
	fi >&2

# vim: ai