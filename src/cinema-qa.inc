// Only compiled by --ui-qa. Real controller input cancels synthetic navigation.
bool qaAborted=false;
void dispatch(const std::string&path,const Json&value){
#ifdef RAFF_UI_QA
 hui::sys::log("QA blocked mutation %s",path.c_str());
#else
 raff::command(path,value);
#endif
}
const char* browse_id(){
#ifdef RAFF_UI_QA
 return "cinema-qa-v102";
#else
 return "classic-v082";
#endif
}
void real_input(const hui::InputFrame&i){
#ifdef RAFF_UI_QA
 if(!qaAborted&&(i.pressed||i.nav!=Direction::none)){qaAborted=true;hui::sys::log("QA ABORT: real controller input; synthetic input disabled");}
#endif
 input(i);
}
void qa_tick(){
#ifdef RAFF_UI_QA
 if(qaAborted)return;
 const int elapsed=frame-240;if(elapsed<0)return;const int step=elapsed/180,at=elapsed%180;
 const char*names[]={"ps2-grid-ar","ps1-grid-en","ps3-grid-ar","psp-grid-en","gba-grid-ar","nds-grid-en","3ds-grid-ar","gamecube-grid-en","dreamcast-grid-ar","arcade-grid-en","platforms-ar","details-en","favorites-ar","local-en"};
 if(step>=14)return;ar=step%2==0;
 if(at==0){detail=-1;platformPicker=false;filterOpen=searching=headerFocus=navFocus=false;gridMode=true;sort=0;
  const int tabs[]={0,4,11,21,18,19,20,22,29,30,0,29,6,7};change_tab(tabs[step]);
  if(step==10){platformPicker=true;platformCursor=20;}
  if(step==11&&!visible.empty())detail=visible[selected];
 }
 if(at==150){screenshotName=std::string("raff-qa-")+names[step]+".bmp";captureRequested=true;hui::sys::log("QA %s tab=%d selected=%d grid=%d",names[step],tab,selected,gridMode);}

#endif
}
