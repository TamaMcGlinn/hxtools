#!/usr/bin/perl

use strict;

&main();

sub oldest_among
{
	my $age = 0;
	my @sb;

	foreach $_ (@_) {
		@sb = stat($_);
		if ($sb[9] > $age) {
			$age = $sb[9];
		}
	}

	return $age;
}

sub set_arc_time
{
	my $file = shift @_;

	system "rm", "-Rf", "tmp";
	mkdir("tmp") || die "mkdir tmp: $!";
	chdir("tmp") || die "chdir tmp: $!";

	my $ret = 0;
	if ($file =~ /\.zip$/i) {
		$ret = system("unzip", "-q", "../$file");
	} elsif ($file =~ /\.7z$/i) {
		$ret = system("7z", "x", "../$file");
	} elsif ($file =~ /\.rar$/i) {
		$ret = system("unrar", "x", "-inul", "../$file");
	} elsif ($file =~ /\.tar.gz$/i) {
		$ret = system("tar", "-xzf", "../$file");
	} elsif ($file =~ /\.cab$/i) {
		$ret = system("cabextract", "-q", "../$file");
	} else {
		print "\e[31m", "Unknown archive format", "\e[0m\n"; # ]]
	}

	if ($ret != 0) {
		print "\e[31m", "Unpack $file failed", "\e[0m\n"; # ]]
	}

	chomp(@_ = `find -type f`);
	my $age = &oldest_among(@_);

	chdir("..") || die "chdir ..: $!";
	system "rm", "-Rf", "tmp";

	if ($age > 0) {
		print "\e[36m", "Setting $file to ", scalar(localtime($age)), "\e[0m\n"; # ]]
		utime(time(), $age, $file);
	} else {
		print "\e[33m", "age = 0", "\e[0m\n"; # ]]
	}
}

sub main
{
	foreach my $version (grep { -d $_ } glob("*"))
	{
		chdir($version) || die "chdir $version: $!";
		print "\e[1m", $version, "\e[0m\n"; # ]]

		my @list = grep { -f $_ && $_ !~ /\.(rpm|bz2)$/ } glob("*");
		foreach my $file (@list) {
			print "\e[32m", "$file", "\e[0m\n"; # ]]
			&set_arc_time($file);
		}

		my $age = &oldest_among(@list);
		if ($age > 0) {
			print "\e[36m", "Setting $version to ", scalar(localtime($age)), "\e[0m\n"; # ]]
			utime(time(), $age, ".");
		} else {
			print "\e[33m", "age = 0", "\e[0m\n"; # ]]
		}
		chdir("..") || die "chdir ..: $!";
	}
}
