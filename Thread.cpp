#include "stdafx.h"
#include "Thread.h"
#include "PioneerBV.h"
#include "PioneerBVDlg.h"
#include "Image.h"
#include "cv.h"
#include "Shared_Memory.h"

extern volatile bool bEndThread;	// Wunsch der GUI an den Thread, 
									// sich zu beenden

double dThreadFrameCount = 0.0;		// Bestimmung der Framerate

// Anzahl der Hilfsbilder
#define ANZ_BILD 9
int iBild_Index = ANZ_BILD;			// Standardanzeige = gegrabbtes Bild

#define PI	3.14159265358979323846f

// Farbkanäle
#define ROT		2
#define GRUEN	1
#define BLAU	0

// Farbkanäle
#define HUE		0
#define SATURATE	1
#define VALUE	2

#define ABS(x) ((x) > 0 ? (x) : (-x))

// Subtraktion zweier Farbkanäle: Fkt: Choose_Diff
int minuend=ROT, subtrahend=GRUEN;

// Init der Werte-Felder
int iValue_0 = 0;
int iValue_1 = 0;
int iValue_2 = 0;
int iValue_3 = 5;
int iValue_4 = 0;
int iValue_5 = 0;
bool debug = false;

// Namen der Variablenfelder
#define FIELD_VALUE_0 0
#define FIELD_VALUE_1 1
#define FIELD_VALUE_2 2
#define FIELD_VALUE_3 3
#define FIELD_VALUE_4 4
#define FIELD_VALUE_5 5

void zeichne_kreuz(long x,long y, long breite, CImage *pImage) ;

/* Die Funktion InitButtons initialisiert die angezeigten Texte 
und Werte der Variablenfelder, sowie die Beschriftung der OwnButtons */
void InitButtons()
{
	// Zeiger auf GUI besorgen
	CPioneerBVDlg* pDlg = ((CPioneerBVApp*)AfxGetApp())->GetDialog();
	
	// Init-Texte der OwnButtons und ihrer Werte
	pDlg->SetButtonDesc( 0, "R-G" );
	minuend=ROT, subtrahend=GRUEN;
	
	pDlg->SetButtonDesc( 1, "Original" );
	iBild_Index = ANZ_BILD;
	
	// Init-Texte und -Werte der Variablenfelder
	pDlg->SetFieldValue( FIELD_VALUE_0, iValue_0);
	pDlg->SetFieldDesc( FIELD_VALUE_0, "bv1 (x)" );
	
	pDlg->SetFieldValue( FIELD_VALUE_1, iValue_1 );
	pDlg->SetFieldDesc( FIELD_VALUE_1, "bv2 (y)" );

	pDlg->SetFieldValue( FIELD_VALUE_2, iValue_2 );
	pDlg->SetFieldDesc( FIELD_VALUE_2, "bv3 (flag)" );

	pDlg->SetFieldValue( FIELD_VALUE_3, iValue_3 );
	pDlg->SetFieldDesc( FIELD_VALUE_3, "Max" );

	pDlg->SetFieldValue( FIELD_VALUE_4, iValue_4 );
	pDlg->SetFieldDesc( FIELD_VALUE_4, "Min" );

	pDlg->SetFieldValue( FIELD_VALUE_5, iValue_5 );
	pDlg->SetFieldDesc( FIELD_VALUE_5, "param5" );
}


/* Die Funktion OnButton wird mit dem Index des gedrueckten
Buttons gerufen, hierbei handelt es sich um die +/- - Buttons
der Variablenfelder, die Variablenwerte werden dabei in der Regel 
auf [0..255] beschränkt  */
void OnButton( CPioneerBVDlg* pDlg, int iButtonIndex )
{
	switch( iButtonIndex )
	{
		// Wert 0 wird geändert
		case 0:
			if (iValue_0 < 255)
			{	iValue_0++;
				pDlg->SetFieldValue( FIELD_VALUE_0, iValue_0 );
			}
			break;
		case 1:
			if (iValue_0 > 0)
			{	iValue_0--;
				pDlg->SetFieldValue( FIELD_VALUE_0, iValue_0 );
			}
			break;

		// Wert 1 wird geändert
		case 2:
			if (iValue_1 < 255)
			{	iValue_1++;
				pDlg->SetFieldValue( FIELD_VALUE_1, iValue_1 );
			}
			break;
		case 3:
			if (iValue_1 > 0)
			{	iValue_1--;
				pDlg->SetFieldValue( FIELD_VALUE_1, iValue_1 );
			}
			break;

		// Wert 2 wird geändert
		case 4:
			if (iValue_2 < 255)
			{	iValue_2++;
				pDlg->SetFieldValue( FIELD_VALUE_2, iValue_2 );
			}
			break;
		case 5:
			if (iValue_2 > 0)
			{	iValue_2--;
				pDlg->SetFieldValue( FIELD_VALUE_2, iValue_2 );
			}
			break;

		// Wert 3 wird geändert
		case 6:
			if (iValue_3 < 255)
			{	iValue_3++;
				pDlg->SetFieldValue( FIELD_VALUE_3, iValue_3 );
			}
			break;
		case 7:
			if (iValue_3 > 0)
			{	iValue_3--;
				pDlg->SetFieldValue( FIELD_VALUE_3, iValue_3 );
			}
			break;

		// Wert 4 wird geändert
		case 8:
			if (iValue_4 < 255)
			{	iValue_4++;
				pDlg->SetFieldValue( FIELD_VALUE_4, iValue_4 );
			}
			break;
		case 9:
			if (iValue_4 > 0)
			{	iValue_4--;
				pDlg->SetFieldValue( FIELD_VALUE_4, iValue_4 );
			}
			break;
			
			// Wert 5 wird geändert
		case 10:
			if (iValue_5 < 255)
			{	iValue_5++;
				pDlg->SetFieldValue( FIELD_VALUE_5, iValue_5 );
			}
			break;
		case 11:
			if (iValue_5 > 0)
			{	iValue_5--;
				pDlg->SetFieldValue( FIELD_VALUE_5, iValue_5 );
			}
			break;
	}
}


/* Die Funktion Choose_Diff kann von einem OwnButton gerufen werden 
und schaltet die globalen Variablen minuend, subtrahend um. Die 
Belegung der Variablen kann im Thread zur Binarisierung mittels
einer Farbkanal-Differenz benutzt werden */

void Choose_Diff(CPioneerBVDlg* pDlg, int OwnButton) {
// zwischen den 6 möglichen Farbanteildifferenzen zirkulieren
// (R-G, R-B, G-R, G-B, B-R, B-G)

		if ((minuend == ROT) && (subtrahend == GRUEN)) {
			minuend = ROT;
			subtrahend = BLAU;
			pDlg->Say("Differenzbild: Rot-Blau");
			pDlg->SetButtonDesc(OwnButton,"R - B");
		}
		else if ((minuend == ROT) && (subtrahend == BLAU)) {
			minuend = GRUEN;
			subtrahend = ROT;
			pDlg->Say("Differenzbild: Grün-Rot");
			pDlg->SetButtonDesc(OwnButton,"G - R");
		}
		else  if((minuend == GRUEN) && (subtrahend == ROT)) {
			minuend = GRUEN;
			subtrahend = BLAU;
			pDlg->Say("Differenzbild: Grün-Blau");
			pDlg->SetButtonDesc(OwnButton,"G - B");
		}
		else if ((minuend == GRUEN) && (subtrahend == BLAU)) {
			minuend = BLAU;
			subtrahend = ROT;
			pDlg->Say("Differenzbild: Blau-Rot");
			pDlg->SetButtonDesc(OwnButton,"B - R");
		}
		else if ((minuend == BLAU) && (subtrahend == ROT)) {
			minuend = BLAU;
			subtrahend = GRUEN;
			pDlg->Say("Differenzbild: Blau-Grün");
			pDlg->SetButtonDesc(OwnButton,"B - G");
		}
		else  if((minuend == BLAU) && (subtrahend == GRUEN)) {
			minuend = ROT;
			subtrahend = GRUEN;
			pDlg->Say("Differenzbild: Rot-Grün");
			pDlg->SetButtonDesc(OwnButton,"R - G");
		}
}

void Choose_Image(CPioneerBVDlg* pDlg, int OwnButton) {
// schaltet die Anzeige der Bilder um
// 0,1 .. ANZ-BILD-1, GrabBild
	char sStr[100];

	// Umschalten
	iBild_Index++;
	if (iBild_Index >ANZ_BILD) iBild_Index = 0;

	// Ausgabe
	if (iBild_Index == ANZ_BILD) {
		pDlg->Say("Anzeige Originalbild");
		pDlg->SetButtonDesc(OwnButton,"Original");
	}
	else {
		sprintf_s(sStr,100,"Anzeige Bild %d",iBild_Index);
		pDlg->Say(sStr);
		sprintf_s(sStr,100,"Bild %d",iBild_Index);
		pDlg->SetButtonDesc(OwnButton,sStr);
	}
}

/* Die Funktion OnOwnButton wird mit dem Index des 
gedrückten OwnButtons gerufen */

void OnButtonOwn( CPioneerBVDlg* pDlg, int iButtonIndex )
{
	switch( iButtonIndex )
	{
		// Knopf 0 gedrueckt
	case 0:
		//pDlg->Say("Knöpche 0");
		Choose_Diff(pDlg,0);
		break;
		// Knopf 1 gedrueckt
	case 1:
		//pDlg->Say("Knöpche 1");
		Choose_Image(pDlg,1);
		break;
		// Knopf 2 gedrueckt
	case 2:
		pDlg->Say("Knöpche 2");
		break;
		// Knopf 3 gedrueckt
	case 3:
		pDlg->Say("Knöpche 3");
		break;
		// Knopf 4 gedrueckt
	case 4:
		pDlg->Say("Knöpche 4");
		break;
	}
}

/* HIER wird gearbeitet ! */

UINT WorkerThread( LPVOID pParam ){ 

	//Speicher reservieren fuer findContours
	CvMemStorage* storage = cvCreateMemStorage();
	CvSeq* first_contour= NULL;

	//Speicher reservieren fuer findContours
	CvMemStorage* storageOrange = cvCreateMemStorage();
	CvSeq* first_contour_orange= NULL;

	HWND hWnd = (HWND)pParam;
	dThreadFrameCount = 0.0;

	Shared_Memory * shared_mem = new Shared_Memory();
	shared_mem->SM_Init();
	 
	int filterCount;	//Filterzaehler fuer Opening
	char sStr[100];		// String für Ausgabe 
	int mitte_x,mitte_y;	// Koordinaten des Schwerpunktes	
	
	CPioneerBVDlg* pDlg;	// GUI
	CCamera* pCamera;		
	CImage* pImage;			// Zeiger auf das Image der Camera
	CImage aImageBlue[ANZ_BILD]; // Array mit vollständigen Bildern
	CImage aImageBlueOrange[ANZ_BILD]; // Array mit vollständigen Bildern
	IplImage *p,*blueImage[ANZ_BILD]; // pIplImage, um ipl-Funktionen zu nutzen
	IplImage *orangeImage[ANZ_BILD]; //Array von Imagezeigern
	IplImage *grauBildBlau;		//grauwert bild fuer binaerbild
	IplImage *grauBildOrange;	//grauwert bild fuer binaerbild

	vector<CvRect> blueRectObjects; //Liste von Objekten als Quadrate//verified
	vector<CvRect> orangeRectObjects; //Liste von Objekten als Quadrate//verified

	CvRect rect; //quadratischer Rahmen eines Objekts
	double realArea; //reale flaeche des Objekts
	double rectArea; //flaeche des Rahmens des Objekts

	double arcLength; //Umfang eines Objekts
	double formFactor; //Formfaktor eines Objekts
	
	//Objektgrossenbegrenzung
	int minAreaBlue=100; //ca. 1,5 meter
	int maxAreaBlue=5000; //ca  25 cm

	int minAreaOrange=50; //ca. 1,5 meter
	int maxAreaOrange=4000; //ca  25 cm

	unsigned int i=0; //schleifenzaehler
	unsigned int j=0;//schleifenzaehler
	bool isInner=false; //Flag fuer gefundenes Objekt


	//blaue Farbwerte
	int blauHueMin=90;
	int blauHueMax=110;

	int blauSatMin=1;
	int blauSatMax=256;

	int blauValMin=0;
	int blauValMax=256;

	//orange Farbwerte
	int orangeHueMin=8;
	int orangeHueMax=35;

	int orangeSatMin=5;
	int orangeSatMax=256;

	int orangeValMin=0;
	int orangeValMax=256;

	// Erkennungsintervalle fuer das Zielobjekt
	CvScalar minColorBlue = cvScalar(blauHueMin,blauSatMin, blauValMin);
	CvScalar maxColorBlue = cvScalar(blauHueMax, blauSatMax, blauValMax);

	CvScalar minColorOrange = cvScalar(orangeHueMin,orangeSatMin, orangeValMin);
	CvScalar maxColorOrange = cvScalar(orangeHueMax, orangeSatMax, orangeValMax);

	//zwischenspeicher fuer Objekterahmen
	CvRect tempRectBlue; 
	CvRect tempRectOrange;	
	
	//Farben fuer die Konturen
	CvScalar aussereFarbe = CV_RGB(0,250,30);
	CvScalar innereFarbe = CV_RGB(250,0,30);
	CvScalar aussereFarbeOrangeObject = CV_RGB(250,250,0);
	
	// Holen des GUI-Zeigers
	pDlg = ((CPioneerBVApp*)AfxGetApp())->GetDialog();
	
	InitButtons();
	
	// Holen der Camera und des Bildes der Camera
	pCamera = pDlg->GetCamera();
	//pCamera ->Start();

	// Viermal ein Bild holen, damit CImage seine beiden Bilder auf die eventuell neue Auflösung umstellen kann
	pImage = pCamera->GetFrame();	pCamera->ReleaseFrame( pImage ); 
	pImage = pCamera->GetFrame();	pCamera->ReleaseFrame( pImage ); 
	pImage = pCamera->GetFrame();	pCamera->ReleaseFrame( pImage ); 
	pImage = pCamera->GetFrame();	pCamera->ReleaseFrame( pImage ); 

	p = pImage->GetImage();

	// Erzeugen gleichgroßer Bildkopien und ihrer pIplImage (Zeiger auf Ipl-Bild)
	for(int b=0;b<ANZ_BILD;b++) {
		aImageBlue[b].Create( pImage->Width(), pImage->Height(), 24 );
		aImageBlueOrange[b].Create( pImage->Width(), pImage->Height(), 24 );

		blueImage[b] = aImageBlue[b].GetImage();
		orangeImage[b] = aImageBlueOrange[b].GetImage();
	}

	grauBildBlau = cvCreateImage(cvGetSize(p), 8, 1);
	grauBildOrange = cvCreateImage(cvGetSize(p), 8, 1);

	sprintf_s(sStr,100,"%d %dx%d Bilder angelegt ...",ANZ_BILD,p->width, p->height);
	pDlg->Say(sStr);
        
	pCamera->ReleaseFrame( pImage );
	
	// Init der Variablenfelder
	iValue_4 = orangeHueMin; //blau Hue min
	iValue_3 = orangeHueMax; //blau Hue max

	pDlg->SetFieldValue( FIELD_VALUE_4, iValue_4 );
	pDlg->SetFieldValue( FIELD_VALUE_3, iValue_3 );
		
	pDlg->Say("Thread gestartet ...");
		
	// und los ...
	while( !bEndThread ){

		isInner=false;
						
		// Holen des aktuellen Bilds
		pImage = pCamera->GetFrame();		// Zeiger auf CImage
		p = pImage->GetImage();				// Zeiger auf IplImage

   		//Vorbehandlung mit dem Medianfilter
		cvSmooth( p, blueImage[8], CV_MEDIAN , 3,3 );
		cvSmooth( p, orangeImage[8], CV_MEDIAN , 3,3 );

		//Binaerbild erstellen
		GetBinaerFromHSV(blueImage[8],grauBildBlau, minColorBlue, maxColorBlue);
		GetBinaerFromHSV(orangeImage[8], grauBildOrange, minColorOrange, maxColorOrange);

		cvCvtColor( grauBildBlau, blueImage[6], CV_GRAY2BGR);
		cvCvtColor( grauBildOrange, orangeImage[6], CV_GRAY2BGR);

		//Objekte isolieren mit Erosion und Dilation
		for(filterCount=1;filterCount>0;filterCount--){
			cvErode( blueImage[6], blueImage[5], NULL, 1 );
			cvDilate( blueImage[5], blueImage[6], NULL, 2 );	
		}

		for(filterCount=1;filterCount>0;filterCount--){
			cvErode( orangeImage[6], orangeImage[5], NULL, 1 );
			cvDilate( orangeImage[5], orangeImage[6], NULL, 1 );	
		}

		//Umwandeln in Grauwertbild
		BgrToGrey(blueImage[6],grauBildBlau);
		BgrToGrey(orangeImage[6],grauBildOrange);

		//fuer ausgabe bzw. Kontrolle auch noch mal als BGR speichern
		cvCvtColor( grauBildBlau, blueImage[3], CV_GRAY2BGR );
		cvCvtColor( grauBildOrange, orangeImage[3], CV_GRAY2BGR );

		//Konturen finden	
		cvFindContours(grauBildBlau,storage,&first_contour,sizeof(CvContour),
		CV_RETR_LIST,CV_CHAIN_APPROX_SIMPLE);

		cvFindContours(grauBildOrange,storageOrange,&first_contour_orange,sizeof(CvContour),
		CV_RETR_LIST,CV_CHAIN_APPROX_SIMPLE);


		//leeren der Listen
		orangeRectObjects.clear();
		blueRectObjects.clear();

		//Konturen Blau durchlaufen
		for( CvSeq* c=first_contour; c!=NULL; c=c->h_next ){

			//berechne Flaeche des aktuellen Objektes
			realArea = ABS(cvContourArea(c));

			if(debug){
				sprintf_s(sStr,100,"Blaues Objekt gefunden mit realArea %10.2f", realArea);
				pDlg->Say(sStr);
			}

			//wenn Flaeche innerhalb der Begrenzung liegt
			if (realArea > minAreaBlue && realArea< maxAreaBlue)
			{

				//berechne Umfang des aktuellen Objektes
				arcLength = cvArcLength(c);
				
				//berechne Formfaktor des aktuellen Objektes
				formFactor = ABS(1/ ((arcLength*arcLength) / (4*PI*realArea )));

				if(formFactor>0.73 && formFactor<0.85){

					//berechne Rahmen des aktuellen Objektes
					rect = cvContourBoundingRect(c);
					rectArea=rect.width*rect.height;

					cvDrawContours( blueImage[3], c, aussereFarbe, innereFarbe, 1, 3, 8 );

					//objekt in vector speichern
					blueRectObjects.push_back(rect);

					if(debug){
						sprintf_s(sStr,100,"Blaues Objekt wurde erkannt mit %10.3f", formFactor);
						pDlg->Say(sStr);
					}

				}else{
					if(debug){
						sprintf_s(sStr,100,"Blaues Objekt wegen Formfaktor NICHT erkannt mit %10.3f", formFactor);
						pDlg->Say(sStr);
					}
				}
			}
		}




		//Konturen orange durchlaufen
		for( CvSeq* c=first_contour_orange; c!=NULL; c=c->h_next ){

			//berechne Flaeche des aktuellen Objektes
			realArea = ABS(cvContourArea(c));

			if(debug){
				sprintf_s(sStr,100,"Oranges Objekt gefunden mit realArea %10.2f", realArea);
				pDlg->Say(sStr);
			}

			//wenn Flaeche innerhalb der Begrenzung liegt
			if (realArea > minAreaOrange && realArea< maxAreaOrange)
			{

				//berechne Umfang des aktuellen Objektes
				arcLength = cvArcLength(c);

				//berechne Formfaktor des aktuellen Objektes
				formFactor = ABS(1/ ((arcLength*arcLength) / (4*PI*realArea )));

				//wenn innerhalb des Formfaktorintervalls
				if(formFactor>0.64 && formFactor<0.81){

					//berechne Rahmen des aktuellen Objektes
					rect = cvContourBoundingRect(c);
					rectArea=rect.width*rect.height;	

					cvDrawContours( blueImage[3], c, aussereFarbeOrangeObject, innereFarbe, 1, 3, 8 );

					//objekt in vector speichern
					orangeRectObjects.push_back(rect);

					if(debug){
						sprintf_s(sStr,100,"Orange Objekt wurde erkannt mit %10.3f", formFactor);
						pDlg->Say(sStr);
					}


				}else{
					if(debug){
						sprintf_s(sStr,100,"Orange Objekt wegen Formfaktor NICHT erkannt mit %10.3f", formFactor);
						pDlg->Say(sStr);
					}

				}
			}
		}


		//durchlaufe alle blauen Elemente
		for(i = 0; blueRectObjects.size() > i ;i++){

			tempRectBlue=blueRectObjects[i];

			//durchlaufe alle orangen Elemente
			for(j = 0; orangeRectObjects.size() > j ;j++){

				tempRectOrange=orangeRectObjects[j];

				if(isRectInner(tempRectOrange,tempRectBlue))
				{
					isInner=true;

					//wenn hier erkannt dann hier 
					//schwerpunkt ausrechnen und setzen
					mitte_x= tempRectOrange.x + tempRectOrange.width / 2;
					mitte_y= tempRectOrange.y + tempRectOrange.height / 2;
					break;
				}
			}
		}

		//setzen des sharedmemory
		if(isInner){

			sprintf_s(sStr,100,"Zielobjekt gefunden! mit Schwerpunkt x=%d y=%d", mitte_x, mitte_y);
			pDlg->Say(sStr);

			zeichne_kreuz(mitte_x,mitte_y,100,pImage);
			shared_mem->SM_SetFloat(1,(float)mitte_x);
			shared_mem->SM_SetFloat(2,(float)mitte_y);
			shared_mem->SM_SetFloat(3,(float)1);
			Sleep(50);
		}else{

			//pDlg->Say("Nichts gefunden!");

			shared_mem->SM_SetFloat(3,0);
		}

		// Das gewünschte Bild anzeigen
		if (iBild_Index == ANZ_BILD) pDlg->ShowImage( pImage,&(aImageBlue[3]));	
		else	pDlg->ShowImage(pImage,&(aImageBlue[iBild_Index]));
		
		// Man muss auch loslassen koennen
		pCamera->ReleaseFrame( pImage );// Frame freigeben
		
		dThreadFrameCount += 1;	
	}
	
	shared_mem->SM_Close();


	::PostMessage( hWnd, WM_WORKER_THREAD_FINISHED, 0, 0 );
	pDlg->Say("Thread gestoppt ...");
	return 0;
}

void zeichne_kreuz(long x,long y, long breite, CImage *pImage) {
  long i,j,sx,sy,BytePerPixel;
  long size;
	
  BYTE *pix = (BYTE*)pImage->GetImage()->imageData; 
		
  sx = pImage->Width();
  sy = pImage->Height();
  BytePerPixel = pImage->Channels();
		
  size = sx*sy*BytePerPixel;
  // horizontale Linie
  for(i=(y*sx+x-breite)*BytePerPixel+BLAU;i<=(y*sx+x+breite)*BytePerPixel+BLAU;i+=BytePerPixel) 
	  if (i>=0  && i< size) pix[i] = 255;
  // vertikale Linie
  for(i=y-breite;i<=y+breite;i++)   {
	  j = (i*sx+x)*BytePerPixel+BLAU;
	  if (j>=0  && j< size) pix[j] = 255;
	  }

  // Zentrum des Kreuzes weiß zeichnen
  pix[(sx*y+x)*BytePerPixel+ROT] = 255;
  pix[(sx*y+x)*BytePerPixel+GRUEN] = 255;
  pix[(sx*y+x)*BytePerPixel+BLAU] = 255;
}


//Binaerbild geben lassen zu einem bestimmten farbintervall
void GetBinaerFromHSV(IplImage* src, IplImage* dstBinaer , CvScalar minColorHSV, CvScalar maxColorHSV)
{
	IplImage* imgHSV = cvCreateImage(cvGetSize(src), 8, 3);

	//Umwandlung RGB nach HSV
	cvCvtColor(src, imgHSV, CV_BGR2HSV);

	// Filtern eines Farbwertes in HSV zu einem Binaerbild
	cvInRangeS(imgHSV, minColorHSV, maxColorHSV, dstBinaer);

	//Speicher freigeben
	cvReleaseImage(&imgHSV);
}


//Konvertieren von BGR 3 kanal zu 1 kanal grauwert
void BgrToGrey(IplImage* img, IplImage* imgGrau)
{
	int xmax,ymax;		// Auflösung des Bildes
	int x,y;			// Laufvariablen durch die Pixel des Bildes
	int zaehlerBGR, zaehlerGrau;			// Pixelzähler

	BYTE *pixelBGR = (BYTE*)img->imageData; // Zeiger auf Pixel erzeugen
	BYTE *pixelGrau = (BYTE*)imgGrau->imageData; // Zeiger auf Pixel erzeugen

	// Auflösung des Bildes bestimmen
	xmax = img->width;
	ymax = img->height;
	zaehlerGrau = 0;
	zaehlerBGR = 0;

	// alle Bildpunkte durchlaufen
	for(y=0;y<ymax;y++)	{
		for (x=0; x < xmax;x++) {

			pixelGrau[zaehlerGrau]=pixelBGR[zaehlerBGR];
			
			zaehlerBGR=zaehlerBGR+3; // nächstes Pixel
			zaehlerGrau++;
		}	
	}		

}

//liegt oranges Rechteck innerhalb des blauen Rechteckes
bool isRectInner(CvRect tempRectOrange, CvRect tempRectBlue)
{
	return tempRectBlue.x < tempRectOrange.x && tempRectBlue.y < tempRectOrange.y 
		&& tempRectBlue.x + tempRectBlue.width > tempRectOrange.x + tempRectOrange.width 
		&& tempRectBlue.y + tempRectBlue.height > tempRectOrange.y + tempRectOrange.height;
}