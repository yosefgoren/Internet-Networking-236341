ID1=$(shell cat id1.buildinfo)
ID2=$(shell cat id2.buildinfo)
USER=$(shell cat user.buildinfo)

IDS=$(ID1)_$(ID2)
IDS_MINUS=$(ID1)-$(ID2)

Submission/$(IDS_MINUS).zip: Submission/$(IDS).pdf Submission/id.txt
	cp -r loadBalancer Submission/loadBalancer
	cd Submission && zip -r $(IDS_MINUS).zip $(IDS).pdf id.txt loadBalancer
	cp $@ /mnt/c/Users/$(USER)/Desktop/

Submission/$(IDS).pdf: Report/main.pdf
	cp $^ $@

Submission/id.txt:
	cd Submission && echo $(ID1) "Yosef Goren" >> id.txt
	cd Submission && echo $(ID2) "Ori Evron" >> id.txt

clean:
	rm -rf Submission/$(IDS).pdf Submission/$(IDS_MINUS).zip

%.buildinfo:
	read -p "Enter content for $@: " __INPUT__ && echo $$__INPUT__ > $@

init: id1.buildinfo id2.buildinfo user.buildinfo
	mkdir -p Report
	mkdir -p Submission
	cd Report && template use texgeneric && mv -n texgeneric main.tex
	cd Report && template use texignore && mv -n texignore .gitignore