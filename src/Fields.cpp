/*
 * Fields.cpp
 *
 * Created: 04/02/2015 11:43:36
 * Author: David
 * Updated for Fusion3: Chandler Mayo
 */ 

#include "Configuration.hpp"
#include "Library/Vector.hpp"
#include "Display.hpp"
#include "PanelDue.hpp"
#include "Hardware/Buzzer.hpp"
#include "Fields.hpp"
#include "Icons/Icons.hpp"

FloatField *currentTemps[maxHeaters], *fpHeightField, *fpLayerHeightField;
FloatField *xPos, *yPos, *zPos;
IntegerButton *activeTemps[maxHeaters], *standbyTemps[maxHeaters];
IntegerButton *spd, *extrusionFactors[maxHeaters], *fanSpeed, *baudRateButton, *volumeButton;
IntegerField *fanRpm, *freeMem, *touchX, *touchY, *fpSizeField, *fpFilamentField;
ProgressBar *printProgressBar;
SingleButton *tabControl, *tabPrint, *tabFiles, *tabMsg, *tabSetup;
SingleButton *moveButton, *extrudeButton, *macroButton;

TextButton *filenameButtons[numDisplayedFiles], *languageButton;
SingleButton *scrollFilesLeftButton, *scrollFilesRightButton, *filesUpButton;
SingleButton *estopbutton, *homeButtons[3], *homeAllButton, *bedCompButton;
SingleButton *iconTemps[maxHeaters], *heaterStates[maxHeaters];
ButtonPress currentExtrudeRatePress, currentExtrudeAmountPress;
StaticTextField *nameField, *statusField, *touchCalibInstruction, *filePopupTitleField;
StaticTextField *messageTextFields[numMessageRows], *messageTimeFields[numMessageRows];
StaticTextField *fwVersionField, *settingsNotSavedField, *areYouSureTextField, *areYouSureQueryField;
ButtonBase *filesButtonField, *pauseButtonField, *resumeButtonField, *resetButtonField;
TextField *timeLeftField;
DisplayField *baseRoot, *commonRoot, *controlRoot, *printRoot, *filesRoot, *messageRoot, *setupRoot;
ButtonBase * null currentTab = NULL;
ButtonPress fieldBeingAdjusted;
ButtonPress currentButton;
PopupWindow *setTempPopup, *movePopup, *extrudePopup, *fileListPopup, *filePopup, *baudPopup, *volumePopup, *areYouSurePopup, *keyboardPopup, *languagePopup;
TextField *zProbe, *fpNameField, *fpGeneratedByField, *userCommandField;

String<machineNameLength> machineName;
String<printingFileLength> printingFile;
String<zprobeBufLength> zprobeBuf;
String<generatedByTextLength> generatedByText;

static const char* const languageNames[] = { "EN", "DE", "FR" };
static_assert(sizeof(languageNames)/sizeof(languageNames[0]) == numLanguages, "Wrong number of languages");
extern const char* const longLanguageNames[] = { "Keyboard EN", "Tastatur DE", "Clavier FR" };
static_assert(sizeof(longLanguageNames)/sizeof(longLanguageNames[0]) == numLanguages, "Wrong number of long languages");

#if DISPLAY_X == 800
const Icon heaterIcons[maxHeaters] = { IconBed, IconNozzle1, IconNozzle2 };
#else
const Icon heaterIcons[maxHeaters] = { IconBed, IconNozzle1, IconNozzle2 };
#endif

namespace Fields
{

	// Add a text button
	TextButton *AddTextButton(PixelNumber row, unsigned int col, unsigned int numCols, const char* array text, Event evt, const char* param)
	{
		PixelNumber width = (DisplayX - 2 * margin + fieldSpacing)/numCols - fieldSpacing;
		PixelNumber xpos = col * (width + fieldSpacing) + margin;
		TextButton *f = new TextButton(row - 2, xpos, width, text, evt, param);
		mgr.AddField(f);
		return f;
	}

	// Add an integer button
	IntegerButton *AddIntegerButton(PixelNumber row, unsigned int col, unsigned int numCols, const char * array null label, const char * array null units, Event evt)
	{
		PixelNumber width = (DisplayX - 2 * margin + fieldSpacing)/numCols - fieldSpacing;
		PixelNumber xpos = col * (width + fieldSpacing) + margin;
		IntegerButton *f = new IntegerButton(row - 2, xpos, width, label, units);
		f->SetEvent(evt, 0);
		mgr.AddField(f);
		return f;
	}
	
	// Add an icon button with a string parameter
	IconButton *AddIconButton(PixelNumber row, unsigned int col, unsigned int numCols, Icon icon, Event evt, const char* param)
	{
		PixelNumber width = (DisplayX - 2 * margin + fieldSpacing)/numCols - fieldSpacing;
		PixelNumber xpos = col * (width + fieldSpacing) + margin;
		IconButton *f = new IconButton(row - 2, xpos, width, icon, evt, param);
		mgr.AddField(f);
		return f;
	}

	// Create a row of test buttons.
	// Optionally, set one to 'pressed' and return that one.
	ButtonPress CreateStringButtonRow(Window * pf, PixelNumber top, PixelNumber left, PixelNumber totalWidth, PixelNumber spacing, unsigned int numButtons,
								const char* array const text[], const char* array const params[], Event evt, int selected = -1)
	{
		const PixelNumber step = (totalWidth + spacing)/numButtons;
		ButtonPress bp;
		for (unsigned int i = 0; i < numButtons; ++i)
		{
			TextButton *tp = new TextButton(top, left + i * step, step - spacing, text[i], evt, params[i]);
			pf->AddField(tp);
			if ((int)i == selected)
			{
				tp->Press(true, 0);
				bp = ButtonPress(tp, 0);
			}
		}
		return bp;
	}

	// Create a popup bar with string parameters
	PopupWindow *CreateStringPopupBar(PixelNumber width, unsigned int numEntries, const char* const text[], const char* const params[], Event ev)
	{
		PopupWindow *pf = new PopupWindow(popupBarHeight, width, popupBackColour);
		DisplayField::SetDefaultColours(popupButtonTextColour, popupButtonBackColour);
		PixelNumber step = (width - 2 * popupSideMargin + popupFieldSpacing)/numEntries;
		for (unsigned int i = 0; i < numEntries; ++i)
		{
			pf->AddField(new TextButton(popupTopMargin, popupSideMargin + i * step, step - popupFieldSpacing, text[i], ev, params[i]));
		}
		return pf;
	}

	// Create a popup bar with integer parameters
	PopupWindow *CreateIntPopupBar(PixelNumber width, unsigned int numEntries, const char* const text[], const int params[], Event ev, Event zeroEv)
	{
		PopupWindow *pf = new PopupWindow(popupBarHeight, width, popupBackColour);
		DisplayField::SetDefaultColours(popupButtonTextColour, popupButtonBackColour);
		PixelNumber step = (width - 2 * popupSideMargin + popupFieldSpacing)/numEntries;
		for (unsigned int i = 0; i < numEntries; ++i)
		{
			pf->AddField(new TextButton(popupSideMargin, popupSideMargin + i * step, step - popupFieldSpacing, text[i], (params[i] == 0) ? zeroEv : ev, params[i]));
		}
		return pf;
	}


/////////////////////////////////////////////

	// Create the grid of heater icons and temperatures
	void CreateTemperatureGrid()
	{
		// Add the labels
		DisplayField::SetDefaultColours(labelTextColour, defaultBackColour);
		mgr.AddField(new StaticTextField(row3 + labelRowAdjust, margin, bedColumn - fieldSpacing - margin, TextAlignment::Right, "current" THIN_SPACE DEGREE_SYMBOL "C"));
		mgr.AddField(new StaticTextField(row4 + labelRowAdjust, margin, bedColumn - fieldSpacing - margin, TextAlignment::Right, "active" THIN_SPACE DEGREE_SYMBOL "C"));
		mgr.AddField(new StaticTextField(row5 + labelRowAdjust, margin, bedColumn - fieldSpacing - margin, TextAlignment::Right, "standby" THIN_SPACE DEGREE_SYMBOL "C"));
	
		// Add the grid
		for (unsigned int i = 0; i < maxHeaters; ++i)
		{
		
			PixelNumber column = ((tempButtonWidth + fieldSpacing) * i) + bedColumn;

			// Add the icon button		
			DisplayField::SetDefaultColours(buttonTextColour, buttonBackColour);
			SingleButton *b = new IconButton(row2, column, tempButtonWidth, heaterIcons[i], evSelectHead, i);
			iconTemps[i] = b;
			mgr.AddField(b);
		
			// Add the current temperature field
			DisplayField::SetDefaultColours(infoTextColour, defaultBackColour);
			FloatField *f = new FloatField(row3 + labelRowAdjust, column, tempButtonWidth, TextAlignment::Centre, 1);
			currentTemps[i] = f;
			mgr.AddField(f);
					

		
			// Add the active temperature button
			DisplayField::SetDefaultColours(buttonTextColour, buttonBackColour);
			IntegerButton *ib = new IntegerButton(row4, column, tempButtonWidth);
			ib->SetEvent(evAdjustActiveTemp, i);
			activeTemps[i] = ib;
			mgr.AddField(ib);

			// Add the standby temperature button
			ib = new IntegerButton(row5, column, tempButtonWidth);
			ib->SetEvent(evAdjustStandbyTemp, i);
			standbyTemps[i] = ib;
			mgr.AddField(ib);
		}
	}
	
	// Create the extra fields for the Control tab
	void CreateControlTabFields()
	{
		mgr.SetRoot(commonRoot);

		DisplayField::SetDefaultColours(infoTextColour, infoBackColour);
		mgr.AddField(xPos = new FloatField(row6p3 + labelRowAdjust, ((DisplayX/2)-((DisplayX/3)/2)) - (DisplayX/3), DisplayX/3, TextAlignment::Centre, 1, "X: "));
		mgr.AddField(yPos = new FloatField(row6p3 + labelRowAdjust, (DisplayX/2)-((DisplayX/3)/2), DisplayX/3, TextAlignment::Centre, 1, "Y: "));
		mgr.AddField(zPos = new FloatField(row6p3 + labelRowAdjust,  ((DisplayX/2)-((DisplayX/3)/2)) + (DisplayX/3), DisplayX/3, TextAlignment::Centre, 2, "Z: "));

		DisplayField::SetDefaultColours(buttonTextColour, notHomedButtonBackColour);
		homeAllButton = AddIconButton(row7p7, 0, 4, IconHomeAll, evSendCommand, "G28");
		homeButtons[0] = AddIconButton(row7p7, 1, 4, IconHomeX, evSendCommand, "G28 X0");
		homeButtons[1] = AddIconButton(row7p7, 2, 4, IconHomeY, evSendCommand, "G28 Y0");
		homeButtons[2] = AddIconButton(row7p7, 3, 4, IconHomeZ, evSendCommand, "G28 Z0");

		DisplayField::SetDefaultColours(buttonTextColour, buttonBackColour);

		moveButton = AddTextButton(row8p7, 0, 3, "Move", evMovePopup, nullptr);
		extrudeButton = AddTextButton(row8p7, 1, 3, "Extrude", evExtrudePopup, nullptr);
		macroButton = AddTextButton(row8p7, 2, 3, "Macro", evListMacros, nullptr);

		controlRoot = mgr.GetRoot();
	}
	
	// Create the fields for the Printing tab
	void CreatePrintingTabFields()
	{
		mgr.SetRoot(commonRoot);
			
		// Labels
		DisplayField::SetDefaultColours(labelTextColour, defaultBackColour);
		mgr.AddField(new StaticTextField(row6 + labelRowAdjust, margin, bedColumn - fieldSpacing, TextAlignment::Right, "extruder" THIN_SPACE "%"));

		// Extrusion factor buttons
		DisplayField::SetDefaultColours(buttonTextColour, buttonBackColour);
		for (unsigned int i = 1; i < maxHeaters; ++i)
		{
			PixelNumber column = ((tempButtonWidth + fieldSpacing) * i) + bedColumn;

			IntegerButton *ib = new IntegerButton(row6, column, tempButtonWidth);
			ib->SetValue(100);
			ib->SetEvent(evExtrusionFactor, i);
			extrusionFactors[i - 1] = ib;
			mgr.AddField(ib);
		}

		// Speed button
		mgr.AddField(spd = new IntegerButton(row7, speedColumn, fanColumn - speedColumn - fieldSpacing, "Speed ", "%"));
		spd->SetValue(100);
		spd->SetEvent(evAdjustSpeed, "M220 S");
			
		// Fan button
		mgr.AddField(fanSpeed = new IntegerButton(row7, fanColumn, pauseColumn - fanColumn - fieldSpacing, "Fan ", "%"));
		fanSpeed->SetEvent(evAdjustFan, 0);

		filesButtonField = new IconButton(row7, pauseColumn, DisplayX - pauseColumn - margin, IconFiles, evListFiles);
		mgr.AddField(filesButtonField);

		DisplayField::SetDefaultColours(buttonTextColour, pauseButtonBackColour);
		pauseButtonField = new TextButton(row7, pauseColumn, DisplayX - pauseColumn - margin, "Pause print", evPausePrint, "M25");
		mgr.AddField(pauseButtonField);

		DisplayField::SetDefaultColours(buttonTextColour, resumeButtonBackColour);
		resumeButtonField = new TextButton(row7, resumeColumn, cancelColumn - resumeColumn - fieldSpacing, "Resume", evResumePrint, "M24");
		mgr.AddField(resumeButtonField);

		DisplayField::SetDefaultColours(buttonTextColour, resetButtonBackColour);
		resetButtonField = new TextButton(row7, cancelColumn, DisplayX - cancelColumn - margin, "Cancel", evReset, "M0");
		mgr.AddField(resetButtonField);
	
		DisplayField::SetDefaultColours(progressBarColour, progressBarBackColour);
		mgr.AddField(printProgressBar = new ProgressBar(row8 + (rowHeight - progressBarHeight)/2, margin, progressBarHeight, DisplayX - 2 * margin));
		mgr.Show(printProgressBar, false);
			
		DisplayField::SetDefaultColours(labelTextColour, defaultBackColour);
		mgr.AddField(timeLeftField = new TextField(row9, margin, DisplayX - 2 * margin, TextAlignment::Left, "Time Estimate: "));
		mgr.Show(timeLeftField, false);

		printRoot = mgr.GetRoot();	
	}
	
	// Create the fields for the Message tab
	void CreateMessageTabFields()
	{
		mgr.SetRoot(baseRoot);
		DisplayField::SetDefaultColours(buttonTextColour, buttonBackColour);
		mgr.AddField(new IconButton(margin,  DisplayX - margin - keyboardButtonWidth, keyboardButtonWidth, IconKeyboard, evKeyboard));
		DisplayField::SetDefaultColours(labelTextColour, defaultBackColour);
		mgr.AddField(new StaticTextField(margin + labelRowAdjust, margin, DisplayX - 2 * margin - keyboardButtonWidth, TextAlignment::Centre, "Messages"));
		PixelNumber row = firstMessageRow;
		for (unsigned int r = 0; r < numMessageRows; ++r)
		{
			StaticTextField *t = new StaticTextField(row, margin, messageTimeWidth, TextAlignment::Left, nullptr);
			mgr.AddField(t);
			messageTimeFields[r] = t;
			t = new StaticTextField(row, messageTextX, messageTextWidth, TextAlignment::Left, nullptr);
			mgr.AddField(t);
			messageTextFields[r] = t;
			row += rowTextHeight;
		}
		messageRoot = mgr.GetRoot();
	}
	
	// Create the fields for the Setup tab
	void CreateSetupTabFields(uint32_t language)
	{
		mgr.SetRoot(baseRoot);
		DisplayField::SetDefaultColours(labelTextColour, defaultBackColour);
		// The firmware version field doubles up as an area for displaying debug messages, so make it the full width of the display

			mgr.AddField(fwVersionField = new StaticTextField(row2, margin, DisplayX, TextAlignment::Centre, "Fusion3 F400 3D Printer"));
						mgr.AddField(fwVersionField = new StaticTextField(row7, margin, DisplayX, TextAlignment::Centre, "For support visit Fusion3Design.com"));

		DisplayField::SetDefaultColours(errorTextColour, errorBackColour);
		mgr.AddField(settingsNotSavedField = new StaticTextField(row3, margin, DisplayX - 2 * margin, TextAlignment::Left, "Some settings are not saved!"));
		settingsNotSavedField->Show(false);

		DisplayField::SetDefaultColours(buttonTextColour, buttonBackColour);
		volumeButton = AddIntegerButton(row4, 0, 3, "Volume ", nullptr, evSetVolume);
		languageButton = AddTextButton(row4, 1, 3, longLanguageNames[language], evSetLanguage, nullptr);
		AddTextButton(row4, 2, 3, "Calibrate touch", evCalTouch, nullptr);
		AddTextButton(row5+5, 0, 3, "Save settings", evSaveSettings, nullptr);
		AddTextButton(row5+5, 1, 3, "Clear settings", evFactoryReset, nullptr);
		AddTextButton(row5+5, 2, 3, "Save & restart", evRestart, nullptr);
			
		DisplayField::SetDefaultColours(labelTextColour, defaultBackColour);
		setupRoot = mgr.GetRoot();
			
		mgr.SetRoot(NULL);
			
		touchCalibInstruction = new StaticTextField(DisplayY/2 - 10, 0, DisplayX, TextAlignment::Centre, nullptr);		// the text is filled in within CalibrateTouch
	}
	
	void CreateIntegerAdjustPopup()
	{
		// Create the popup window used to adjust temperatures, fan speed, extrusion factor etc.
		static const char* const tempPopupText[] = {"-5", "-1", "Set", "+1", "+5"};
		static const int tempPopupParams[] = { -5, -1, 0, 1, 5 };
		setTempPopup = CreateIntPopupBar(tempPopupBarWidth, 5, tempPopupText, tempPopupParams, evAdjustInt, evSetInt);
	}

	// Create the movement popup window
	void CreateMovePopup()
	{
		static const char * array xyJogValues[] = { "-100", "-10", "-1", "-0.1", "0.1",  "1", "10", "100" };
		static const char * array zJogValues[] = { "-50", "-5", "-0.5", "-0.05", "0.05",  "0.5", "5", "50" };

		movePopup = new PopupWindow(movePopupHeight, movePopupWidth, popupBackColour);
		PixelNumber ypos = popupTopMargin;
		DisplayField::SetDefaultColours(popupTextColour, popupBackColour);
		movePopup->AddField(new StaticTextField(ypos + labelRowAdjust, popupSideMargin, movePopupWidth - 2 * popupSideMargin, TextAlignment::Centre, "Move head"));
		ypos += buttonHeight + moveButtonRowSpacing;
		const PixelNumber xpos = popupSideMargin + axisLabelWidth;
		movePopup->AddField(new StaticTextField(ypos + labelRowAdjust, popupSideMargin, axisLabelWidth, TextAlignment::Left, "X"));
		DisplayField::SetDefaultColours(popupButtonTextColour, popupButtonBackColour);
		CreateStringButtonRow(movePopup, ypos, xpos, movePopupWidth - xpos - popupSideMargin, fieldSpacing, 8, xyJogValues, xyJogValues, evMoveX);
		ypos += buttonHeight + moveButtonRowSpacing;
		DisplayField::SetDefaultColours(popupTextColour, popupBackColour);
		movePopup->AddField(new StaticTextField(ypos + labelRowAdjust, popupSideMargin, axisLabelWidth, TextAlignment::Left, "Y"));
		DisplayField::SetDefaultColours(popupButtonTextColour, popupButtonBackColour);
		CreateStringButtonRow(movePopup, ypos, xpos, movePopupWidth - xpos - popupSideMargin, fieldSpacing, 8, xyJogValues, xyJogValues, evMoveY);
		ypos += buttonHeight + moveButtonRowSpacing;
		DisplayField::SetDefaultColours(popupTextColour, popupBackColour);
		movePopup->AddField(new StaticTextField(ypos + labelRowAdjust, popupSideMargin, axisLabelWidth, TextAlignment::Left, "Z"));
		DisplayField::SetDefaultColours(popupButtonTextColour, popupButtonBackColour);
		CreateStringButtonRow(movePopup, ypos, xpos, movePopupWidth - xpos - popupSideMargin, fieldSpacing, 8, zJogValues, zJogValues, evMoveZ);
		ypos += buttonHeight + moveButtonRowSpacing;
		PixelNumber doneButtonWidth = movePopupWidth/4;
		movePopup->AddField(new IconButton(ypos, (movePopupWidth - doneButtonWidth)/2, doneButtonWidth, IconCancel, evCancel));
	}
	
	// Create the extrusion controls popup
	void CreateExtrudePopup()
	{
		static const char * array extrudeAmountValues[] = { "100", "50", "20", "10", "5",  "1" };
		static const char * array extrudeSpeedValues[] = { "50", "40", "20", "10", "5" };
		static const char * array extrudeSpeedParams[] = { "3000", "2400", "1200", "600", "300" };

		extrudePopup = new PopupWindow(extrudePopupHeight, extrudePopupWidth, popupBackColour);
		PixelNumber ypos = popupTopMargin;
		DisplayField::SetDefaultColours(popupTextColour, popupBackColour);
		extrudePopup->AddField(new StaticTextField(ypos + labelRowAdjust, popupSideMargin, extrudePopupWidth - 2 * popupSideMargin, TextAlignment::Centre, "Extrusion amount (mm)"));
		ypos += buttonHeight + extrudeButtonRowSpacing;
		DisplayField::SetDefaultColours(popupButtonTextColour, popupButtonBackColour);
		currentExtrudeAmountPress = CreateStringButtonRow(extrudePopup, ypos, popupSideMargin, extrudePopupWidth - 2 * popupSideMargin, fieldSpacing, 6, extrudeAmountValues, extrudeAmountValues, evExtrudeAmount, 3);
		ypos += buttonHeight + extrudeButtonRowSpacing;
		DisplayField::SetDefaultColours(popupTextColour, popupBackColour);
		extrudePopup->AddField(new StaticTextField(ypos + labelRowAdjust, popupSideMargin, extrudePopupWidth - 2 * popupSideMargin, TextAlignment::Centre, "Speed (mm/sec)"));
		ypos += buttonHeight + extrudeButtonRowSpacing;
		DisplayField::SetDefaultColours(popupButtonTextColour, popupButtonBackColour);
		currentExtrudeRatePress = CreateStringButtonRow(extrudePopup, ypos, popupSideMargin, extrudePopupWidth - 2 * popupSideMargin, fieldSpacing, 5, extrudeSpeedValues, extrudeSpeedParams, evExtrudeRate, 4);
		ypos += buttonHeight + extrudeButtonRowSpacing;
		extrudePopup->AddField(new TextButton(ypos, popupSideMargin, extrudePopupWidth/3 - 2 * popupSideMargin, "Extrude", evExtrude));
		extrudePopup->AddField(new IconButton(ypos, extrudePopupWidth/3 + popupSideMargin, extrudePopupWidth/3 - 2 * popupSideMargin, IconCancel, evCancel));
		extrudePopup->AddField(new TextButton(ypos, (2 * extrudePopupWidth)/3 + popupSideMargin, extrudePopupWidth/3 - 2 * popupSideMargin, "Retract", evRetract));
	}
	
	// Create the popup used to list files and macros
	void CreateFileListPopup()
	{
		fileListPopup = new PopupWindow(fileListPopupHeight, fileListPopupWidth, popupBackColour);
		const PixelNumber navButtonWidth = fileListPopupWidth/8;
		const PixelNumber backButtonPos = fileListPopupWidth - navButtonWidth - popupSideMargin;
		const PixelNumber upButtonPos = backButtonPos - navButtonWidth - fieldSpacing;
		const PixelNumber rightButtonPos = upButtonPos - navButtonWidth - fieldSpacing;
		const PixelNumber leftButtonPos = popupSideMargin;
		const PixelNumber textPos = popupSideMargin + navButtonWidth;

		DisplayField::SetDefaultColours(popupTextColour, popupBackColour);
		fileListPopup->AddField(filePopupTitleField = new StaticTextField(popupTopMargin + labelRowAdjust, textPos, rightButtonPos - textPos, TextAlignment::Centre, nullptr));

		DisplayField::SetDefaultColours(popupButtonTextColour, popupButtonBackColour);
		fileListPopup->AddField(scrollFilesLeftButton = new TextButton(popupTopMargin, leftButtonPos, navButtonWidth, "<", evScrollFiles, -numFileRows));
		scrollFilesLeftButton->Show(false);
		fileListPopup->AddField(scrollFilesRightButton = new TextButton(popupTopMargin, rightButtonPos, navButtonWidth, ">", evScrollFiles, numFileRows));
		scrollFilesRightButton->Show(false);
		
		fileListPopup->AddField(filesUpButton = new IconButton(popupTopMargin, upButtonPos, navButtonWidth, IconUp, evNull));
		filesUpButton->Show(false);
		fileListPopup->AddField(new IconButton(popupTopMargin, backButtonPos, navButtonWidth, IconCancel, evCancel));
		
		const PixelNumber fileFieldWidth = (fileListPopupWidth + fieldSpacing - (2 * popupSideMargin))/numFileColumns;
		unsigned int fileNum = 0;
		for (unsigned int c = 0; c < numFileColumns; ++c)
		{
			PixelNumber row = popupTopMargin;
			for (unsigned int r = 0; r < numFileRows; ++r)
			{
				row += buttonHeight + fileButtonRowSpacing;
				TextButton *t = new TextButton(row, (fileFieldWidth * c) + popupSideMargin, fileFieldWidth - fieldSpacing, nullptr, evNull);
				t->Show(false);
				fileListPopup->AddField(t);
				filenameButtons[fileNum] = t;
				++fileNum;
			}
		}
	}

	// Create the popup window used to display the file dialog
	void CreateFileActionPopup()
	{
		filePopup = new PopupWindow(fileInfoPopupHeight, fileInfoPopupWidth, popupBackColour);
		DisplayField::SetDefaultColours(popupTextColour, popupBackColour);

		fpNameField = new TextField(popupTopMargin, popupSideMargin, fileInfoPopupWidth - 2 * popupSideMargin, TextAlignment::Left, "Filename: ");
		fpSizeField = new IntegerField(popupTopMargin + rowTextHeight, popupSideMargin, fileInfoPopupWidth - 2 * popupSideMargin, TextAlignment::Left, "Size: ", " bytes");
		fpLayerHeightField = new FloatField(popupTopMargin + 2 * rowTextHeight, popupSideMargin, fileInfoPopupWidth - 2 * popupSideMargin, TextAlignment::Left, 1, "Layer height: ","mm");
		fpHeightField = new FloatField(popupTopMargin + 3 * rowTextHeight, popupSideMargin, fileInfoPopupWidth - 2 * popupSideMargin, TextAlignment::Left, 1, "Object height: ", "mm");
		fpFilamentField = new IntegerField(popupTopMargin + 4 * rowTextHeight, popupSideMargin, fileInfoPopupWidth - 2 * popupSideMargin, TextAlignment::Left, "Filament needed: ", "mm");
		fpGeneratedByField = new TextField(popupTopMargin + 5 * rowTextHeight, popupSideMargin, fileInfoPopupWidth - 2 * popupSideMargin, TextAlignment::Left, "Sliced by: ", generatedByText.c_str());
		filePopup->AddField(fpNameField);
		filePopup->AddField(fpSizeField);
		filePopup->AddField(fpLayerHeightField);
		filePopup->AddField(fpHeightField);
		filePopup->AddField(fpFilamentField);
		filePopup->AddField(fpGeneratedByField);

		DisplayField::SetDefaultColours(popupButtonTextColour, popupButtonBackColour);
		filePopup->AddField(new TextButton(popupTopMargin + 7 * rowTextHeight, popupSideMargin, fileInfoPopupWidth/3 - 2 * popupSideMargin, "Print", evPrint));
		filePopup->AddField(new IconButton(popupTopMargin + 7 * rowTextHeight, fileInfoPopupWidth/3 + popupSideMargin, fileInfoPopupWidth/3 - 2 * popupSideMargin, IconCancel, evCancel));
		filePopup->AddField(new IconButton(popupTopMargin + 7 * rowTextHeight, (2 * fileInfoPopupWidth)/3 + popupSideMargin, fileInfoPopupWidth/3 - 2 * popupSideMargin, IconTrash, evDeleteFile));
	}

	// Create the "Are you sure?" popup
	void CreateAreYouSurePopup()
	{
		areYouSurePopup = new PopupWindow(areYouSurePopupHeight, areYouSurePopupWidth, popupBackColour);
		DisplayField::SetDefaultColours(popupTextColour, popupBackColour);
		areYouSurePopup->AddField(areYouSureTextField = new StaticTextField(popupSideMargin, margin, areYouSurePopupWidth - 2 * margin, TextAlignment::Centre, nullptr));
		areYouSurePopup->AddField(areYouSureQueryField = new StaticTextField(popupTopMargin + rowHeight, margin, areYouSurePopupWidth - 2 * margin, TextAlignment::Centre, nullptr));

		DisplayField::SetDefaultColours(popupButtonTextColour, popupButtonBackColour);
		areYouSurePopup->AddField(new IconButton(popupTopMargin + 2 * rowHeight, popupSideMargin, areYouSurePopupWidth/2 - 2 * popupSideMargin, IconOk, evYes));
		areYouSurePopup->AddField(new IconButton(popupTopMargin + 2 * rowHeight, areYouSurePopupWidth/2 + 10, areYouSurePopupWidth/2 - 2 * popupSideMargin, IconCancel, evCancel));
	}

	// Create the baud rate adjustment popup
	void CreateBaudRatePopup()
	{
		static const char* const baudPopupText[] = { "9600", "19200", "38400", "57600", "115200" };
		static const int baudPopupParams[] = { 9600, 19200, 38400, 57600, 115200 };
		baudPopup = CreateIntPopupBar(fullPopupWidth, 5, baudPopupText, baudPopupParams, evAdjustBaudRate, evAdjustBaudRate);
	}

	// Create the volume adjustment popup
	void CreateVolumePopup()
	{
		static const char* const volumePopupText[Buzzer::MaxVolume + 1] = { "Off", "1", "2", "3", "4", "5" };
		static const int volumePopupParams[Buzzer::MaxVolume + 1] = { 0, 1, 2, 3, 4, 5 };
		volumePopup = CreateIntPopupBar(fullPopupWidth, Buzzer::MaxVolume + 1, volumePopupText, volumePopupParams, evAdjustVolume, evAdjustVolume);
	}
	
	// Create the language popup (currently only affects the keyboard layout)
	void CreateLanguagePopup()
	{
		static const int languagePopupParams[numLanguages] = { 0, 1, 2 };
		languagePopup = CreateIntPopupBar(fullPopupWidth, numLanguages, languageNames, languagePopupParams, evAdjustLanguage, evAdjustLanguage);
	}
	
	// Create the pop-up keyboard
	void CreateKeyboardPopup(uint32_t language)
	{
		static const char* array const keysGB[4] = { "1234567890-+", "QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM./" };
		static const char* array const keysDE[4] = { "1234567890-+", "QWERTZUIOP", "ASDFGHJKL", "YXCVBNM./" };
		static const char* array const keysFR[4] = { "1234567890-+", "AZERTWUIOP", "QSDFGHJKLM", "YXCVBN./" };
		static const char* array const * const keyboards[numLanguages] = { keysGB, keysDE, keysFR };

		keyboardPopup = new PopupWindow(keyboardPopupHeight, keyboardPopupWidth, popupBackColour);
		if (language >= numLanguages)
		{
			language = 0;
		}
		const char* array const * array const keys = keyboards[language];
		DisplayField::SetDefaultColours(popupButtonTextColour, popupButtonBackColour);
		PixelNumber row = keyboardPopupTopMargin;
		for (size_t i = 0; i < 4; ++i)
		{
			PixelNumber column = popupSideMargin + (i * keyButtonHStep)/3;
			const char * s = keys[i];
			while (*s != 0)
			{
				keyboardPopup->AddField(new CharButton(row, column, keyButtonWidth, *s, evKey));
				++s;
				column += keyButtonHStep;
			}
			switch (i)
			{
			case 1:
				keyboardPopup->AddField(new IconButton(row, keyboardPopupWidth - popupSideMargin - 2 * keyButtonWidth, 2 * keyButtonWidth, IconBackspace, evBackspace));
				break;
				
			case 2:
				keyboardPopup->AddField(new IconButton(row, keyboardPopupWidth - popupSideMargin - 2 * keyButtonWidth, 2 * keyButtonWidth, IconUp, evUp));
				break;

			case 3:
				keyboardPopup->AddField(new IconButton(row, keyboardPopupWidth - popupSideMargin - 2 * keyButtonWidth, 2 * keyButtonWidth, IconDown, evDown));
				break;
			
			default:
				break;
			}
			row += keyButtonVStep;
		}
			
		// Add the cancel, space and enter keys
		const PixelNumber keyButtonHSpace = keyButtonHStep - keyButtonWidth;
		const PixelNumber wideKeyButtonWidth = (keyboardPopupWidth - 2 * popupSideMargin - 2 * keyButtonHSpace)/4;
		keyboardPopup->AddField(new IconButton(row, popupSideMargin, wideKeyButtonWidth, IconCancel, evCancel));		
		keyboardPopup->AddField(new TextButton(row, popupSideMargin + wideKeyButtonWidth + keyButtonHSpace, 2 * wideKeyButtonWidth, nullptr, evKey, (int)' '));	
		keyboardPopup->AddField(new IconButton(row, popupSideMargin + 3 * wideKeyButtonWidth + 2 * keyButtonHSpace, wideKeyButtonWidth, IconEnter, evSendKeyboardCommand));
		
		row += keyButtonVStep;
		
		// Add the text area in which the command is built
		DisplayField::SetDefaultColours(popupInfoTextColour, popupInfoBackColour);
		userCommandField = new TextField(row, popupSideMargin, keyboardPopupWidth - 2 * popupSideMargin, TextAlignment::Left, nullptr, "_");
		keyboardPopup->AddField(userCommandField);
	}

	// Create all the fields we ever display
	void CreateFields(uint32_t language)
	{
		mgr.Init(defaultBackColour);
		DisplayField::SetDefaultFont(DEFAULT_FONT);
		ButtonWithText::SetFont(DEFAULT_FONT);
		SingleButton::SetTextMargin(textButtonMargin);
		SingleButton::SetIconMargin(iconButtonMargin);
	
		// Create the fields that are displayed on all pages
		DisplayField::SetDefaultColours(buttonTextColour, buttonBackColour, buttonBorderColour, buttonGradColour, buttonPressedBackColour, buttonPressedGradColour);
		tabControl = AddTextButton(rowTabs, 0, 4, "Control", evTabControl, nullptr);
		tabPrint = AddTextButton(rowTabs, 1, 4, "Print", evTabPrint, nullptr);
		tabMsg = AddTextButton(rowTabs, 2, 4, "Console", evTabMsg, nullptr);
		tabSetup = AddTextButton(rowTabs, 3, 4, "Setup", evTabSetup, nullptr);
		baseRoot = mgr.GetRoot();		// save the root of fields that we usually display
		DisplayField::SetDefaultColours(buttonTextColour, buttonBackColour);


		// Create the fields that are common to the Control and Print pages
		DisplayField::SetDefaultColours(titleBarTextColour, titleBarBackColour);
		mgr.AddField(nameField = new StaticTextField(row1, 0, DisplayX - statusFieldWidth, TextAlignment::Centre, machineName.c_str()));
		mgr.AddField(statusField = new StaticTextField(row1, DisplayX - statusFieldWidth, statusFieldWidth, TextAlignment::Right, nullptr));
		CreateTemperatureGrid();
		commonRoot = mgr.GetRoot();		// save the root of fields that we display on more than one page

		lcd.drawBitmap(140, (DisplayY/2-19), 200, 39, FusionLogo);
		lcd.setColor(UTFT::fromRGB(65,65,65));
		lcd.fillRect(0,250,480,270);
		lcd.setColor(orange);

		for (unsigned int i = 2; i < 478; ++i) // Progress bar on the loading screen.
		{
			lcd.fillRect((i-1),252,i,268);
			delay_ms(5);
		}

		delay_ms(500);
		
		// Create the pages
		CreateControlTabFields();
		CreatePrintingTabFields();
		CreateMessageTabFields();
		CreateSetupTabFields(language);

		// Create the popup fields
		CreateIntegerAdjustPopup();
		CreateMovePopup();
		CreateExtrudePopup();
		CreateFileListPopup();
		CreateFileActionPopup();
		CreateVolumePopup();
		CreateBaudRatePopup();
		CreateAreYouSurePopup();
		CreateKeyboardPopup(language);
		CreateLanguagePopup();

		// Set initial values
		for (unsigned int i = 0; i < maxHeaters; ++i)
		{
			currentTemps[i]->SetValue(0.0);
			activeTemps[i]->SetValue(0.0);
			standbyTemps[i]->SetValue(0.0);
			extrusionFactors[i]->SetValue(100);
		}

		xPos->SetValue(0.0);
		yPos->SetValue(0.0);
		zPos->SetValue(0.0);
		fanSpeed->SetValue(0);
		fanRpm->SetValue(0);
		spd->SetValue(100);
	}
	
	void SettingsAreSaved(bool areSaved)
	{
		mgr.Show(settingsNotSavedField, !areSaved);
	}
	
	void ShowFilesButton()
	{
		mgr.Show(resumeButtonField, false);
		mgr.Show(resetButtonField, false);
		mgr.Show(pauseButtonField, false);
		mgr.Show(filesButtonField, true);
	}
	
	void ShowPauseButton()
	{
		mgr.Show(resumeButtonField, false);
		mgr.Show(resetButtonField, false);
		mgr.Show(filesButtonField, false);
		mgr.Show(pauseButtonField, true);
	}
	
	void ShowResumeAndCancelButtons()
	{
		mgr.Show(pauseButtonField, false);
		mgr.Show(filesButtonField, false);
		mgr.Show(resumeButtonField, true);
		mgr.Show(resetButtonField, true);
	}
}

// End
