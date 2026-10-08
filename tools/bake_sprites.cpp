// tools/bake_sprites.cpp Ã¢â‚¬â€ Horneado de sprites isomÃƒÂ©tricos desde modelos 3D CC0
// Personajes: KayKit Adventurers (github.com/KayKit-Game-AssetS) Ã¢â‚¬â€ CC0
// Armas: mismo pack (Assets/gltf), acopladas al joint handslot.{r,l} por frame.
// Salida: assets/units/<unidad>_<anim>_<equipo>.png + assets/units/sheets.txt
//         build/units_contact.png (hoja de contacto de verificaciÃƒÂ³n)
// Uso:   bake_sprites.exe   (cwd = raÃƒÂ­z del repo)
#include "raylib.h"
#include "raymath.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <string>
#include <vector>
#ifdef _WIN32
#include <direct.h>
#endif

static const char* SRC = "assets_src/KayKit-Character-Pack-Adventures-1.0-main/"
                         "addons/kaykit_character_pack_adventures/";
static const char* OUT_DIR = "assets/units/";

// Encuadre isomÃƒÂ©trico (pitch fijo, cÃƒÂ¡mara orbita por direcciÃƒÂ³n)
static const int   FW = 160, FH = 200;     // tamaÃƒÂ±o de frame
static const float CAM_DIST = 5.35f;
static const float CAM_PITCH = 32.f;
static const float FOV = 30.f;
static const float YAW_OFFSET = 0.f;
static const int   DIRS = 8;

// handslot.* (joints del rig) respecto a su padre hand.{r,l}: TRS local extraido del GLB
static const Vector3 SLOT_T      = {0.0f, 0.096125066f, -0.057500124f};
static const Quaternion SLOT_R_R = {1.61e-9f, -1.61e-9f, 0.7071068f, 0.7071067f};
static const Quaternion SLOT_R_L = {1.61e-9f, 1.61e-9f, -0.7071068f, 0.7071067f};

static const char* LIGHT_VS =
    "#version 330\n"
    "in vec3 vertexPosition; in vec3 vertexNormal; in vec2 vertexTexCoord;\n"
    "uniform mat4 mvp; uniform mat4 matModel;\n"
    "out vec3 vNormal; out vec2 vTexCoord; out vec3 vPos;\n"
    "void main(){ vPos=vec3(matModel*vec4(vertexPosition,1.0));\n"
    "  vNormal=normalize(mat3(matModel)*vertexNormal); vTexCoord=vertexTexCoord;\n"
    "  gl_Position=mvp*vec4(vertexPosition,1.0); }\n";

static const char* LIGHT_FS =
    "#version 330\n"
    "in vec3 vNormal; in vec2 vTexCoord; in vec3 vPos;\n"
    "uniform sampler2D texture0; uniform vec4 colDiffuse;\n"
    "uniform vec3 lightDir; uniform vec3 lightColor; uniform float ambient;\n"
    "out vec4 finalColor;\n"
    "void main(){\n"
    "  vec3 n=normalize(vNormal);\n"
    "  float diff=max(dot(n,-normalize(lightDir)),0.0);\n"
    "  vec4 tex=texture(texture0,vTexCoord);\n"
    "  vec3 base=tex.rgb*colDiffuse.rgb;\n"
    "  finalColor=vec4(base*(ambient+(1.0-ambient)*lightColor*diff), tex.a*colDiffuse.a);\n"
    "}\n";

// raylib solo aloja animVertices/boneWeights en primitivas con JOINTS/WEIGHTS;
// las mallas estaticas del GLB llegan NULL y UpdateModelAnimation crashea (rmodels.c:2330).
static void PatchAnimBuffers(Model& model){
    for(int m=0;m<model.meshCount;m++){
        Mesh& ms = model.meshes[m];
        const int v3 = ms.vertexCount*3, v4 = ms.vertexCount*4;
        if(!ms.animVertices){
            ms.animVertices = (float*)RL_CALLOC(v3,sizeof(float));
            if(ms.vertices) memcpy(ms.animVertices,ms.vertices,v3*sizeof(float));
        }
        if(!ms.animNormals){
            ms.animNormals = (float*)RL_CALLOC(v3,sizeof(float));
            if(ms.normals) memcpy(ms.animNormals,ms.normals,v3*sizeof(float));
        }
        if(!ms.boneWeights) ms.boneWeights = (float*)RL_CALLOC(v4,sizeof(float));
        if(!ms.boneIds)     ms.boneIds     = (unsigned char*)RL_CALLOC(v4,1);
    }
}

struct AnimDef { const char* outName; const char* clip; int frames; };
static const AnimDef ANIMS[] = {
    {"idle","Idle",4},{"walk","Walking_A",8},
    {"attack","1H_Melee_Attack_Chop",6},{"death","Death_A",8},
    {nullptr,nullptr,0}
};

// Recetas: unidad de juego -> personaje + armas (R=mano derecha, L=izquierda)
struct Recipe { const char* unit; const char* charFile; const char* charKey;
                const char* wR; const char* wL; };
static const Recipe RECIPES[] = {
    {"spear",     "Knight.glb",      "knight",   "arrow",              nullptr},
    {"axe",       "Barbarian.glb",   "barbarian","axe_1handed",        "shield_round_barbarian"},
    {"poleaxe",   "Barbarian.glb",   "barbarian","axe_2handed",        nullptr},
    {"dismounted","Knight.glb",      "knight",   "sword_1handed",      "shield_square"},
    {"bow",       "Rogue.glb",       "rogue",    "crossbow_1handed",   nullptr},
    {"crossbow",  "Rogue.glb",       "rogue",    "crossbow_2handed",   nullptr},
    {"mounted",   "Knight.glb",      "knight",   "sword_1handed",      nullptr},
    {"knights",   "Knight.glb",      "knight",   "sword_2handed",      nullptr},
};

struct Team { const char* tag; Color tint; };
static const Team TEAMS[2] = {
    {"p", {168,196,255,255}},   // jugador: frio (azulado)
    {"e", {255,178,162,255}},   // enemigo: calido
};

struct Loaded { std::string path; Model model; };
static std::vector<Loaded> g_chars, g_weapons;

static Model* GetChar(const char* file){
    std::string p = std::string(SRC)+"Characters/gltf/"+file;
    for(auto& c: g_chars) if(c.path==p) return &c.model;
    Model m = LoadModel(p.c_str());
    if(m.meshCount==0){ fprintf(stderr,"ERROR: no carga %s\n",p.c_str()); return nullptr; }
    PatchAnimBuffers(m);
    g_chars.push_back({p,m});
    return &g_chars.back().model;
}
static Model* GetWeapon(const char* name){
    std::string p = std::string(SRC)+"Assets/gltf/"+name+".gltf";
    for(auto& w: g_weapons) if(w.path==p) return &w.model;
    Model m = LoadModel(p.c_str());
    if(m.meshCount==0){ fprintf(stderr,"ERROR: no carga arma %s\n",p.c_str()); return nullptr; }
    PatchAnimBuffers(m);
    g_weapons.push_back({p,m});
    return &g_weapons.back().model;
}

static int FindClip(ModelAnimation* anims,int n,const char* name){
    for(int i=0;i<n;i++) if(strcmp(anims[i].name,name)==0) return i;
    for(int i=0;i<n;i++) if(strstr(anims[i].name,name)!=nullptr) return i;
    return -1;
}
static int FindBone(const Model& m,const char* name){
    for(int i=0;i<m.boneCount;i++) if(strcmp(m.bones[i].name,name)==0) return i;
    return -1;
}

// Transform global de un handslot = parent(hand) global Ã¢Ë†Ëœ local(T,R)
struct SlotPose { Vector3 pos; Quaternion rot; };
static SlotPose GetSlot(const Transform* poses,int handIdx,bool right){
    const Transform h = poses[handIdx];
    SlotPose s;
    s.pos = Vector3Add(h.translation, Vector3RotateByQuaternion(SLOT_T,h.rotation));
    s.rot = QuaternionMultiply(h.rotation, right? SLOT_R_R : SLOT_R_L);
    return s;
}
// pre-rot local del arma en el espacio del socket (para alinear ejes del mesh)
static Quaternion WeaponPreRot(const char* name){
    if(name && strcmp(name,"arrow")==0){
        // arrow: eje largo = X -> gira a +Y (vertical)
        return {0.f, 0.f, 0.f, 1.f};   // identidad: eje Y ya alineado por el socket
    }
    return {0.f, 0.f, 0.f, 1.f};
}
static float WeaponScale(const char* name){
    if(name && strcmp(name,"arrow")==0) return 1.8f;   // arrow -> lanza
    return 1.0f;
}
static void DrawWeaponAt(const SlotPose& s, Model* w, const char* wname){
    if(!w) return;
    Vector3 axis = {0,1,0}; float ang = 0;
    Quaternion final = QuaternionMultiply(s.rot, WeaponPreRot(wname));
    QuaternionToAxisAngle(final,&axis,&ang);
    float sc = WeaponScale(wname);
    DrawModelEx(*w, s.pos, axis, ang*RAD2DEG, {sc,sc,sc}, WHITE);
}

int main(){
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(320,240,"bake_sprites");
    SetTargetFPS(120);

    Shader sh = LoadShaderFromMemory(LIGHT_VS,LIGHT_FS);
    g_chars.reserve(8); g_weapons.reserve(8);   // punteros estables (sin realloc)
    int locDir   = GetShaderLocation(sh,"lightDir");
    int locColor = GetShaderLocation(sh,"lightColor");
    int locAmb   = GetShaderLocation(sh,"ambient");
    float amb = 0.42f;
    SetShaderValue(sh,locAmb,&amb,SHADER_UNIFORM_FLOAT);

    RenderTexture2D rt = LoadRenderTexture(FW,FH);

#ifdef _WIN32
    _mkdir(OUT_DIR);
#else
    mkdir(OUT_DIR,0755);
#endif
    FILE* meta = fopen(TextFormat("%ssheets.txt",OUT_DIR),"w");
    if(!meta){ fprintf(stderr,"ERROR: no puedo escribir %ssheets.txt\n",OUT_DIR); CloseWindow(); return 1; }
    fprintf(meta,"# bake_sprites - KayKit Adventurers (CC0) - frame %dx%d pitch %.0f dist %.1f yawOff %.0f\n",
            FW,FH,CAM_PITCH,CAM_DIST,YAW_OFFSET);
    fprintf(meta,"# layout: X=frames (cols), Y=direcciones (8 filas, i=yaw i*45+%.0f)\n",YAW_OFFSET);
    fprintf(meta,"# equipos p=jugador e=enemigo; handslot.r/l (TRS local respecto hand.*)\n");
    fprintf(meta,"# unit char wR wL\n");
    for(const Recipe& r: RECIPES)
        fprintf(meta,"# %s %s %s %s\n", r.unit, r.charKey, r.wR?r.wR:"-", r.wL?r.wL:"-");
    fprintf(meta,"# unit anim team frameW frameH cols dirs\n");

    // Hoja de contacto: filas = unidadÃƒâ€”equipo, columnas = direcciones (frame0 del anim en curso)
    const int cellW=90, cellH=112;
    const int rows = (int)(sizeof(RECIPES)/sizeof(RECIPES[0]))*2;
    Image contact = GenImageColor(cellW*DIRS, cellH*rows, (Color){18,20,26,255});

    int row=0;
    for(const Recipe& rec: RECIPES){
        Model* ch = GetChar(rec.charFile);
        if(!ch) continue;
        int animCount=0;
        ModelAnimation* anims = LoadModelAnimations(
            (std::string(SRC)+"Characters/gltf/"+rec.charFile).c_str(),&animCount);
        if(animCount==0){ fprintf(stderr,"ERROR: sin animaciones %s\n",rec.charFile); continue; }

        int handR = FindBone(*ch,"hand.r");
        int handL = FindBone(*ch,"hand.l");
        if(handR<0 || handL<0){ fprintf(stderr,"ERROR: sin hand.* en %s\n",rec.charFile); continue; }
        Model* wR = rec.wR? GetWeapon(rec.wR) : nullptr;
        Model* wL = rec.wL? GetWeapon(rec.wL) : nullptr;

        for(const Team& team: TEAMS){
            for(int m=0;m<ch->materialCount;m++)
                ch->materials[m].maps[MATERIAL_MAP_ALBEDO].color = team.tint;
            if(wR) for(int m=0;m<wR->materialCount;m++)
                wR->materials[m].maps[MATERIAL_MAP_ALBEDO].color = team.tint;
            if(wL) for(int m=0;m<wL->materialCount;m++)
                wL->materials[m].maps[MATERIAL_MAP_ALBEDO].color = team.tint;

            for(const AnimDef* a=ANIMS; a->outName; ++a){
                int clip = FindClip(anims,animCount,a->clip);
                if(clip<0){ fprintf(stderr,"WARN: clip '%s' no encontrado en %s\n",a->clip,rec.charFile); continue; }
                int cf = anims[clip].frameCount;
                int cols = a->frames;

                Image sheet = GenImageColor(FW*cols, FH*DIRS, (Color){0,0,0,0});
                for(int d=0; d<DIRS; d++){
                    float yaw = YAW_OFFSET + d*(360.f/DIRS);
                    float rad = yaw*(float)PI/180.f;
                    float pr  = CAM_PITCH*(float)PI/180.f;
                    Vector3 center = {0,0.95f,0};
                    Vector3 camPos = {
                        center.x + CAM_DIST*std::sin(rad)*std::cos(pr),
                        center.y + CAM_DIST*std::sin(pr),
                        center.z + CAM_DIST*std::cos(rad)*std::cos(pr) };
                    // Luz pegada a la cÃƒÂ¡mara (izquierda-superior) para lectura uniforme por direcciÃƒÂ³n
                    Vector3 ld = {-0.45f*std::cos(rad)+0.62f*std::sin(rad), 0.75f,
                                  -0.62f*std::cos(rad)-0.45f*std::sin(rad)};
                    Vector3 lc = {1.0f,0.97f,0.90f};
                    SetShaderValue(sh,locDir,&ld,SHADER_UNIFORM_VEC3);
                    SetShaderValue(sh,locColor,&lc,SHADER_UNIFORM_VEC3);

                    for(int f=0; f<cols; f++){
                        int srcFrame = (cf<=cols)? f : (int)((long)f*cf/cols);
                        if(srcFrame>=cf) srcFrame=cf-1;
                        UpdateModelAnimation(*ch,anims[clip],srcFrame);
                        const Transform* poses = anims[clip].framePoses[srcFrame];
                        SlotPose sr = GetSlot(poses,handR,true);
                        SlotPose sl = GetSlot(poses,handL,false);

                        BeginTextureMode(rt);
                            ClearBackground({0,0,0,0});
                            BeginMode3D((Camera3D){ camPos, center, {0,1,0}, FOV, CAMERA_PERSPECTIVE });
                                DrawCircle3D({0,0.01f,0},0.42f,{1,0,0},90.f,{0,0,0,110});
                                if(!getenv("BAKE_WEAPON_ONLY")) DrawModel(*ch,{0,0,0},1.0f,WHITE);
                                DrawWeaponAt(sr,wR,rec.wR);
                                DrawWeaponAt(sl,wL,rec.wL);
                            EndMode3D();
                        EndTextureMode();

                        Image frame = LoadImageFromTexture(rt.texture);
                        ImageFlipVertical(&frame);
                        Rectangle src = {0,0,(float)frame.width,(float)frame.height};
                        Rectangle dst = {(float)(f*FW),(float)(d*FH),(float)FW,(float)FH};
                        ImageDraw(&sheet,frame,src,dst,WHITE);
                        UnloadImage(frame);
                    }
                    // Celda de contacto: frame0 de la fila=direccion d
                    Image preview = ImageCopy(sheet);
                    Rectangle ps = {0,(float)(d*FH),(float)FW,(float)FH};
                    ImageCrop(&preview,ps);
                    ImageResize(&preview,cellW,cellH);
                    Rectangle pd = {(float)(d*cellW),(float)(row*cellH),(float)cellW,(float)cellH};
                    ImageDraw(&contact,preview,{0,0,(float)cellW,(float)cellH},pd,WHITE);
                    UnloadImage(preview);
                }
                char rel[256];
                snprintf(rel,sizeof(rel),"%s%s_%s_%s.png",OUT_DIR,rec.unit,a->outName,team.tag);
                ExportImage(sheet,rel);
                fprintf(meta,"%s %s %s %d %d %d %d\n",rec.unit,a->outName,team.tag,
                        FW,FH,cols,DIRS);
                printf("OK %s\n",rel);
                UnloadImage(sheet);
            }
            row++;
        }
        UnloadModelAnimations(anims,animCount);
    }
    fclose(meta);
    ExportImage(contact,"build/units_contact.png");
    UnloadImage(contact);

    for(auto& c: g_chars) UnloadModel(c.model);
    for(auto& w: g_weapons) UnloadModel(w.model);
    UnloadRenderTexture(rt);
    UnloadShader(sh);
    CloseWindow();
    printf("bake completado\n");
    return 0;
}
