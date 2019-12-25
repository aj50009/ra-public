#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb/stb_image.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/mat4x4.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <unordered_map>
#include <initializer_list>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <cstddef>
#include <cassert>
#include <cstdlib>
#include <ctime>

#define VERSION_STRING "#version 430 core"

static GLFWwindow* window = nullptr;
static int width = 640, height = 480;
static glm::vec3 cameracenter(0.0f);
static glm::vec3 camerapos(0.0f, 3.0f, 10.0f);
static glm::vec3 lightpos(5.0f, 5.0f, -5.0f);
static double xprev = 0.0, yprev = 0.0;
static GLuint phong_prog = 0;
#define phong_projViewModel_uniform 0
#define phong_modelNormal_uniform 1
#define phong_model_uniform 2
#define phong_campos_uniform 3
#define phong_lightpos_uniform 4
#define phong_texdiff_uniform 5
#define phong_texnorm_uniform 6
#define phong_texspec_uniform 7
#define phong_usetexdiff_uniform 8
#define phong_usetexnorm_uniform 9
#define phong_usetexspec_uniform 10

static GLuint compileshaderdefs(GLenum shadertype, const std::string& sourcepath, const std::unordered_map<std::string, std::string>& defs, const std::string& verstr = VERSION_STRING) {

    /* open source file stream */
    std::ifstream sourcestream(sourcepath, std::ios::in | std::ios::binary);
    assert(sourcestream.good());

    /* tell source file length */
    sourcestream.seekg(0, std::ios::end);
    GLint sourcelen = static_cast<GLint>(sourcestream.tellg());
    sourcestream.seekg(0, std::ios::beg);

    /* read and close stream */
    std::vector<GLchar> source;
    source.reserve(static_cast<std::size_t>(sourcelen));
    sourcestream.read(reinterpret_cast<char*>(source.data()), static_cast<std::streamsize>(sourcelen));
    sourcestream.close();

    /* create shader and prepare buffers */
    GLuint shader = glCreateShader(shadertype);
    const GLchar* verbuffer = reinterpret_cast<const GLchar*>(verstr.c_str());
    GLint verbufflen = verstr.size();
    std::stringstream ss_defsrc;
    ss_defsrc << std::endl;
    for (const std::pair<std::string, std::string>& defPair : defs)
        ss_defsrc << "#define " << defPair.first << " " << defPair.second << std::endl;
    std::string defsrc = ss_defsrc.str();
    const GLchar* defsrcbuffer = reinterpret_cast<const GLchar*>(defsrc.c_str());
    GLint defsrcbufflen = defsrc.size();
    const GLchar* sourcebuffer = source.data();

    /* load and compile shader */
    const GLchar* buffers[] = { verbuffer, defsrcbuffer, sourcebuffer };
    GLint bufflens[] = { verbufflen, defsrcbufflen, sourcelen };
    #define nbuffers (sizeof(buffers) / sizeof(*buffers))
    glShaderSource(shader, nbuffers, buffers, bufflens);
    glCompileShader(shader);

    /* assure successful shader compilation */
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        GLint infologlen;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologlen);
        std::vector<GLchar> infolog;
        infolog.reserve(static_cast<std::size_t>(infologlen));
        glGetShaderInfoLog(shader, static_cast<GLsizei>(infologlen), nullptr, infolog.data());
        std::cerr << reinterpret_cast<char*>(infolog.data()) << std::endl;
        std::exit(EXIT_FAILURE);
    }

    /* return handle */
    return shader;

}

static GLuint compileshader(GLenum shadertype, const std::string& sourcepath, const std::string& verstr = VERSION_STRING) {

    /* create empty defines map and compile shader with it */
    std::unordered_map<std::string, std::string> defs;
    return compileshaderdefs(shadertype, sourcepath, defs, verstr);

}

static GLuint linkprogram(const std::initializer_list<GLuint>& shaders) {

    /* create program and attach shaders, then link */
    GLuint program = glCreateProgram();
    for (GLuint shader : shaders)
        glAttachShader(program, shader);
    glLinkProgram(program);

    /* assure successful program linkage */
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        GLint infologlen;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infologlen);
        std::vector<GLchar> infolog;
        infolog.reserve(static_cast<std::size_t>(infologlen));
        glGetProgramInfoLog(program, static_cast<GLsizei>(infologlen), nullptr, infolog.data());
        std::cerr << reinterpret_cast<char*>(infolog.data()) << std::endl;
        std::exit(EXIT_FAILURE);
    }

    /* return handle */
    return program;

}

static GLuint loadtex(const std::string& imgpath) {

    /* ask stb_image to flip on y axis */
    stbi_set_flip_vertically_on_load(true);

    /* load image */
    int x, y, nc;
    unsigned char* data = stbi_load(imgpath.c_str(), &x, &y, &nc, 4);
    assert(data != nullptr);

    /* generate and bind texture */
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    /* set texture params */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    /* load texture and free image */
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);

    /* generate mipmap */
    glGenerateMipmap(GL_TEXTURE_2D);

    /* unbind and return texture */
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;

}

/* particle data */
typedef struct __attribute__((packed)) {
    float x, y, z;
    float vx, vy, vz;
    float ax, ay, az;
    float scale;
    float initialx, initialy, initialz;
    float initialvx, initialvy, initialvz;
} particle;

static GLuint genparticletex() {

    /* generate and bind texture */
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_RECTANGLE, tex);

    /* set texture params */
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    /* allocate particle data */
    #define nparticles 8192
    std::vector<particle> data;
    data.reserve(nparticles);
    particle* pdata = data.data();

    /* generate initial data */
    #define rand01() ((std::rand() % 10001) / 10000.0f)
    #define randrange(s, e) ((s) + ((e) - (s)) * rand01())
    for (int i = 0; i < nparticles; ++i) {
        particle& p = pdata[i];
        p.x = p.initialx = randrange(-5.5f, 5.5f);
        p.y = randrange(0.0f, 5.5f);
        p.initialy = randrange(4.5f, 6.0f);
        p.z = p.initialz = randrange(-5.5f, 5.5f);
        p.vx = p.initialvx = randrange(-0.5f, 0.5f);
        p.vy = p.initialvy = randrange(-0.5f, -0.15f);
        p.vz = p.initialvz = randrange(-0.5f, 0.5f);
        p.ax = randrange(-0.015f, 0.015f);
        p.ay = randrange(-0.015f, -0.001f);
        p.az = randrange(-0.015f, 0.015f);
        p.scale = randrange(0.025f, 0.085f);
    }

    /* load texture */
    #define szparticle (sizeof(particle) / sizeof(float))
    glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_R32F, szparticle, nparticles, 0, GL_RED, GL_FLOAT, pdata);

    /* unbind and return texture */
    glBindTexture(GL_TEXTURE_RECTANGLE, 0);
    return tex;

}

static GLuint genparticletexempty() {

    /* generate and bind texture */
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_RECTANGLE, tex);

    /* set texture params */
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    /* set texture size */
    glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_R32F, szparticle, nparticles, 0, GL_RED, GL_FLOAT, nullptr);

    /* unbind and return texture */
    glBindTexture(GL_TEXTURE_RECTANGLE, 0);
    return tex;

}

static GLuint genparticlefbo(GLuint particletex) {
    
    /* generate and bind fbo */
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    /* bind texture to framebuffer color attachment */
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, particletex, 0);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    /* unbind and return fbo */
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return fbo;

}

static void swapparticletex(GLuint& particletex, GLuint& particletex2, GLuint particlefbo) {

    /* swap values */
    GLuint tmp = particletex, tmp2 = particletex2;
    particletex = tmp2;
    particletex2 = tmp;

    /* bind fbo */
    glBindFramebuffer(GL_FRAMEBUFFER, particlefbo);

    /* bind texture to framebuffer color attachment */
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tmp, 0);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    /* unbind fbo */
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

/* model data */
typedef struct {
    GLuint vao;
    GLuint texdiff, texnorm, texspec;
    int nindices;
} model;

/* scene data */
typedef struct {
    std::unordered_map<std::string, GLuint> texdata;
    std::vector<model> models;
    glm::vec3 pos;
    glm::quat rot;
    glm::vec3 scale;
} scene;

/* vertex attribute data */
#define sz_pos_attrib 3
#define sz_uv_attrib 2
#define sz_norm_attrib 3
#define sz_tng_attrib 3
#define sz_bitng_attrib 3
#define sz_total_attrib (sz_pos_attrib + sz_uv_attrib + sz_norm_attrib + sz_tng_attrib + sz_bitng_attrib)
typedef struct __attribute__((packed)) {
    float pos[sz_pos_attrib];
    float uv[sz_uv_attrib];
    float norm[sz_norm_attrib];
    float tng[sz_tng_attrib];
    float bitng[sz_bitng_attrib];
} attribs;

/* load scene */
static scene loadscene(const std::string& filepath, const std::unordered_map<std::string, std::string>& texmap) {

    /* create scene */
    scene s;
    s.pos = glm::vec3(0.0f);
    s.rot = glm::identity<glm::quat>();
    s.scale = glm::vec3(1.0f);

    /* load all textures */
    for (const std::pair<std::string, std::string>& pair : texmap)
        s.texdata[pair.first] = loadtex(pair.second);

    /* import scene */
    Assimp::Importer a_importer;
    a_importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_COLORS | aiComponent_BONEWEIGHTS | aiComponent_ANIMATIONS | aiComponent_TEXTURES | aiComponent_LIGHTS | aiComponent_CAMERAS);
    const aiScene* a_scene = a_importer.ReadFile(filepath, aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices | aiProcess_Triangulate | aiProcess_RemoveComponent | aiProcess_GenSmoothNormals | aiProcess_PreTransformVertices | aiProcess_RemoveRedundantMaterials);
    assert(a_scene != nullptr);

    /* load all meshes */
    for (int i = 0; i < a_scene->mNumMeshes; ++i) {
        
        /* create model struct */
        model m;
        const aiMesh* a_mesh = a_scene->mMeshes[i];

        /* gen and bind vao & vbo & ibo */
        GLuint vbo, ibo;
        glGenVertexArrays(1, &(m.vao));
        glBindVertexArray(m.vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ibo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

        /* build vertices buffer */
        std::vector<attribs> arrayattribs;
        arrayattribs.reserve(a_mesh->mNumVertices);
        attribs* parrayattribs = arrayattribs.data();
        for (int j = 0; j < a_mesh->mNumVertices; ++j) {
            parrayattribs[j].pos[0] = a_mesh->mVertices[j].x;
            parrayattribs[j].pos[1] = a_mesh->mVertices[j].y;
            parrayattribs[j].pos[2] = a_mesh->mVertices[j].z;
            if (a_mesh->HasTextureCoords(0)) {
                parrayattribs[j].uv[0] = a_mesh->mTextureCoords[0][j].x;
                parrayattribs[j].uv[1] = a_mesh->mTextureCoords[0][j].y;
            } else
                parrayattribs[j].uv[0] = parrayattribs[j].uv[1] = 0.0f;
            parrayattribs[j].norm[0] = a_mesh->mNormals[j].x;
            parrayattribs[j].norm[1] = a_mesh->mNormals[j].y;
            parrayattribs[j].norm[2] = a_mesh->mNormals[j].z;
            parrayattribs[j].tng[0] = a_mesh->mTangents[j].x;
            parrayattribs[j].tng[1] = a_mesh->mTangents[j].y;
            parrayattribs[j].tng[2] = a_mesh->mTangents[j].z;
            parrayattribs[j].bitng[0] = a_mesh->mBitangents[j].x;
            parrayattribs[j].bitng[1] = a_mesh->mBitangents[j].y;
            parrayattribs[j].bitng[2] = a_mesh->mBitangents[j].z;
        }

        /* load vertices buffer, then free vector */
        glBufferData(GL_ARRAY_BUFFER, a_mesh->mNumVertices * sizeof(attribs), parrayattribs, GL_STATIC_DRAW);
        arrayattribs.clear();

        /* build indices buffer */
        std::vector<GLushort> indices;
        m.nindices = 3 * a_mesh->mNumFaces;
        indices.reserve(m.nindices);
        GLushort* pindices = indices.data();
        for (int j = 0; j < a_mesh->mNumFaces; ++j) {
            assert(a_mesh->mFaces[j].mNumIndices == 3);
            pindices[3 * j] = a_mesh->mFaces[j].mIndices[0];
            pindices[3 * j + 1] = a_mesh->mFaces[j].mIndices[1];
            pindices[3 * j + 2] = a_mesh->mFaces[j].mIndices[2];
        }

        /* load indices buffer, then free vector */
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m.nindices * sizeof(GLushort), pindices, GL_STATIC_DRAW);
        indices.clear();

        /* map input attributes */
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(3);
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(0, sz_pos_attrib, GL_FLOAT, GL_FALSE, sizeof(attribs), reinterpret_cast<const void*>(0));
        glVertexAttribPointer(1, sz_uv_attrib, GL_FLOAT, GL_FALSE, sizeof(attribs), reinterpret_cast<const void*>(sz_pos_attrib * sizeof(GLfloat)));
        glVertexAttribPointer(2, sz_norm_attrib, GL_FLOAT, GL_FALSE, sizeof(attribs), reinterpret_cast<const void*>((sz_pos_attrib + sz_uv_attrib) * sizeof(GLfloat)));
        glVertexAttribPointer(3, sz_tng_attrib, GL_FLOAT, GL_FALSE, sizeof(attribs), reinterpret_cast<const void*>((sz_pos_attrib + sz_uv_attrib + sz_norm_attrib) * sizeof(GLfloat)));
        glVertexAttribPointer(4, sz_bitng_attrib, GL_FLOAT, GL_FALSE, sizeof(attribs), reinterpret_cast<const void*>((sz_pos_attrib + sz_uv_attrib + sz_norm_attrib + sz_tng_attrib) * sizeof(GLfloat)));

        /* get material ptr */
        assert(a_mesh->mMaterialIndex < a_scene->mNumMaterials);
        const aiMaterial* a_mat = a_scene->mMaterials[a_mesh->mMaterialIndex];
        
        /* no textures if material is null */
        if (a_mat == nullptr)
            m.texdiff = m.texnorm = m.texspec = 0;
        else {

            /* check if diffuse texture exists and find its handle */
            if (a_mat->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
                aiString texname;
                a_mat->GetTexture(aiTextureType_DIFFUSE, 0, &texname);
                assert(s.texdata.count(texname.C_Str()) > 0);
                m.texdiff = s.texdata[texname.C_Str()];
            } else
                m.texdiff = 0;

            /* check if normal texture exists and find its handle */
            if (a_mat->GetTextureCount(aiTextureType_NORMALS) > 0) {
                aiString texname;
                a_mat->GetTexture(aiTextureType_NORMALS, 0, &texname);
                assert(s.texdata.count(texname.C_Str()) > 0);
                m.texnorm = s.texdata[texname.C_Str()];
            } else
                m.texnorm = 0;

            /* check if specular texture exists and find its handle */
            if (a_mat->GetTextureCount(aiTextureType_SPECULAR) > 0) {
                aiString texname;
                a_mat->GetTexture(aiTextureType_SPECULAR, 0, &texname);
                assert(s.texdata.count(texname.C_Str()) > 0);
                m.texspec = s.texdata[texname.C_Str()];
            } else
                m.texspec = 0;

        }

        /* push model */
        s.models.push_back(m);

    }

    /* unbind vao and return scene */
    glBindVertexArray(0);
    return s;

}

/* draw scene */
static void drawscene(const scene& s) {
    
    /* build matrices */
    glm::mat4 projView = glm::perspective(glm::pi<float>() / 4.0f, static_cast<float>(width) / height, 0.5f, 25.0f) * glm::lookAt(camerapos, cameracenter, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 modelMatrix = glm::translate(s.pos) * glm::toMat4(s.rot) * glm::scale(s.scale);
    glm::mat4 projViewModel = projView * modelMatrix;
    glm::mat3 modelNormal = glm::transpose(glm::mat3(glm::inverse(modelMatrix)));

    /* use phong program */
    glUseProgram(phong_prog);

    /* bind matrix and vector uniforms */
    glUniformMatrix4fv(phong_projViewModel_uniform, 1, GL_FALSE, glm::value_ptr(projViewModel));
    glUniformMatrix3fv(phong_modelNormal_uniform, 1, GL_FALSE, glm::value_ptr(modelNormal));
    glUniformMatrix4fv(phong_model_uniform, 1, GL_FALSE, glm::value_ptr(modelMatrix));
    glUniform3fv(phong_campos_uniform, 1, glm::value_ptr(camerapos));
    glUniform3fv(phong_lightpos_uniform, 1, glm::value_ptr(lightpos));

    /* draw all models */
    for (const model& m : s.models) {

        /* bind diffuse texture */
        if (m.texdiff == 0)
            glUniform1i(phong_usetexdiff_uniform, GL_FALSE);
        else {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m.texdiff);
            glUniform1i(phong_texdiff_uniform, 0);
            glUniform1i(phong_usetexdiff_uniform, GL_TRUE);
        }

        /* bind normal texture */
        if (m.texnorm == 0)
            glUniform1i(phong_usetexnorm_uniform, GL_FALSE);
        else {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, m.texnorm);
            glUniform1i(phong_texnorm_uniform, 1);
            glUniform1i(phong_usetexnorm_uniform, GL_TRUE);
        }

        /* bind specular texture */
        if (m.texspec == 0)
            glUniform1i(phong_usetexspec_uniform, GL_FALSE);
        else {
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, m.texspec);
            glUniform1i(phong_texspec_uniform, 2);
            glUniform1i(phong_usetexspec_uniform, GL_TRUE);
        }

        /* bind vao */
        glBindVertexArray(m.vao);

        /* draw call */
        glDrawElements(GL_TRIANGLES, m.nindices, GL_UNSIGNED_SHORT, nullptr);

    }

    /* unbind vao */
    glBindVertexArray(0);

}

int main(int argc, char** argv) {

    /* seed c random engine */
    std::srand(std::time(nullptr));

    /* init glfw lib */
    int result = glfwInit();
    assert(result != GLFW_FALSE);

    /* hint requested gl 4.3 core (forward-comapt) */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    /* hint 4x msaa antialiasing */
    glfwWindowHint(GLFW_SAMPLES, 4);

    /* create glfw window */
    window = glfwCreateWindow(width, height, "ra", nullptr, nullptr);
    assert(window != nullptr);
    glfwMakeContextCurrent(window);

    /* glad load core 4.3 extensions */
    result = gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress));
    assert(result != 0);
    
    /* window resize callback */
    glfwSetWindowSizeCallback(window,
        [](GLFWwindow*, int newwidth, int newheight) {
            width = newwidth;
            height = newheight;
        });

    /* cursor callback */
    glfwGetCursorPos(window, &xprev, &yprev);
    glfwSetCursorPosCallback(window,
        [](GLFWwindow*, double x, double y) {
            float dx = x - xprev, dy = y - yprev;
            xprev = x;
            yprev = y;
            camerapos = glm::rotate(10.0f * dx / width, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(camerapos, 1.0f);
        });

    /* particle shader defines */
    std::unordered_map<std::string, std::string> defs;
    std::stringstream ss_defs;
    ss_defs << nparticles;
    defs["nparticles"] = ss_defs.str();
    ss_defs.str("");
    ss_defs << szparticle;
    defs["szparticle"] = ss_defs.str();

    /* compile particle shaders */
    GLuint particles_vs = compileshader(GL_VERTEX_SHADER, "particles_vs.glsl");
    GLuint particles_gs = compileshader(GL_GEOMETRY_SHADER, "particles_gs.glsl");
    GLuint particles_fs = compileshader(GL_FRAGMENT_SHADER, "particles_fs.glsl");
    GLuint particles_prog = linkprogram({ particles_vs, particles_gs, particles_fs });
    #define proj_uniform 0
    #define view_uniform 1
    #define model_uniform 2
    #define particleTex_uniform 3
    #define flakeTex_uniform 4

    /* compile compute shaders */
    GLuint compute_vs = compileshader(GL_VERTEX_SHADER, "compute_vs.glsl");
    GLuint compute_gs = compileshader(GL_GEOMETRY_SHADER, "compute_gs.glsl");
    GLuint compute_fs = compileshaderdefs(GL_FRAGMENT_SHADER, "compute_fs.glsl", defs);
    GLuint compute_prog = linkprogram({ compute_vs, compute_gs, compute_fs });
    #define deltaTime_uniform 5
    #define minY_uniform 6

    /* compile phong shaders */
    GLuint phong_vs = compileshader(GL_VERTEX_SHADER, "phong_vs.glsl");
    GLuint phong_fs = compileshader(GL_FRAGMENT_SHADER, "phong_fs.glsl");
    phong_prog = linkprogram({ phong_vs, phong_fs });

    /* load terrain */
    std::unordered_map<std::string, std::string> map;
    map["terraindiff.jpg"] = "terraindiff.jpg";
    map["terrainnorm.jpg"] = "terrainnorm.jpg";
    scene terrain = loadscene("terrain.dae", map);
    terrain.pos = glm::vec3(0.0f, -1.5f, 0.0f);
    terrain.scale = glm::vec3(20.0f);

    /* manually set terrain textures (just in case) */
    assert(terrain.models.size() >= 1);
    terrain.models[0].texdiff = terrain.texdata["terraindiff.jpg"];
    terrain.models[0].texnorm = terrain.texdata["terrainnorm.jpg"];

    /* compile gradient shaders */
    GLuint gradient_vs = compileshader(GL_VERTEX_SHADER, "gradient_vs.glsl");
    GLuint gradient_fs = compileshader(GL_FRAGMENT_SHADER, "gradient_fs.glsl");
    GLuint gradient_prog = linkprogram({ gradient_vs, gradient_fs });

    /* gradient mesh vertices along with color values */
    #define gradcolor0 0.8f,0.91f,1.0f
    #define gradcolor1 1.0f,0.937f,0.745f
    #define gradcolor2 0.992f,0.722f,0.557f
    #define gradlimit 0.325f
    GLfloat gradient[] = {
        -1.0f, 1.0f, gradcolor0,
        -1.0f, gradlimit, gradcolor1,
        1.0f, gradlimit, gradcolor1,
        -1.0f, 1.0f, gradcolor0,
        1.0f, gradlimit, gradcolor1,
        1.0f, 1.0f, gradcolor0,
        -1.0f, gradlimit, gradcolor1,
        -1.0f, -1.0f, gradcolor2,
        1.0f, -1.0f, gradcolor2,
        -1.0f, gradlimit, gradcolor1,
        1.0f, -1.0f, gradcolor2,
        1.0f, gradlimit, gradcolor1
    };
    #define szgradient (sizeof(*gradient))
    #define ngradient (sizeof(gradient) / szgradient)

    /* generate gradient vao & vbo */
    GLuint gradvao, gradvbo;
    glGenVertexArrays(1, &gradvao);
    glBindVertexArray(gradvao);
    glGenBuffers(1, &gradvbo);
    glBindBuffer(GL_ARRAY_BUFFER, gradvbo);
    glBufferData(GL_ARRAY_BUFFER, ngradient * szgradient, gradient, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * szgradient, reinterpret_cast<const void*>(0));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * szgradient, reinterpret_cast<const void*>(2 * szgradient));
    glBindVertexArray(0);

    /* load flake texture */
    GLuint flaketex = loadtex("flake.png");

    /* generate particle data textures */
    GLuint particletex = genparticletex(), particletex2 = genparticletexempty();

    /* generate particle framebuffer */
    GLuint particlefbo = genparticlefbo(particletex2);

    /* create and bind vao */
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    /* map dummy attribute data in vao */
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 1, GL_BYTE, GL_FALSE, 0, nullptr);

    /* enable depth testing */
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    /* enable face culling */
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    /* enable samples (antialiasing) */
    glEnable(GL_MULTISAMPLE);

    /* reset glfw timer */
    glfwSetTime(0.0);

    /* window event loop */
    while (!glfwWindowShouldClose(window)) {

        /* set display viewport */
        glViewport(0, 0, width, height);

        /* clear default renderbuffer */
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        /* draw terrain */
        drawscene(terrain);

        /* bind particle vao */
        glBindVertexArray(vao);

        /* bind display program */
        glUseProgram(particles_prog);

        /* display program matrices */
        glm::mat4 proj = glm::perspective(glm::pi<float>() / 4.0f, static_cast<float>(width) / height, 0.5f, 25.0f);
        glm::mat4 view = glm::lookAt(camerapos, cameracenter, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 model = glm::identity<glm::mat4>();
        glUniformMatrix4fv(proj_uniform, 1, GL_FALSE, glm::value_ptr(proj));
        glUniformMatrix4fv(view_uniform, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        
        /* bind display program flake texture */
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, flaketex);
        glUniform1i(flakeTex_uniform, 0);

        /* bind display program particle data */
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_RECTANGLE, particletex);
        glUniform1i(particleTex_uniform, 1);

        /* draw snow particles */
        glDrawArraysInstanced(GL_POINTS, 0, 1, nparticles);

        /* bind gradient vao & use gradient program to draw gradient */
        glBindVertexArray(gradvao);
        glUseProgram(gradient_prog);
        glDrawArrays(GL_TRIANGLES, 0, ngradient);

        /* swap buffers */
        glfwSwapBuffers(window);

        /* bind particle fbo */
        glBindFramebuffer(GL_FRAMEBUFFER, particlefbo);

        /* set compute viewport */
        glViewport(0, 0, szparticle, nparticles);

        /* bind compute program */
        glUseProgram(compute_prog);

        /* bind display program particle data */
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_RECTANGLE, particletex);
        glUniform1i(particleTex_uniform, 0);

        /* bind delta time and min y limit */
        glUniform1f(deltaTime_uniform, static_cast<float>(glfwGetTime()));
        glUniform1f(minY_uniform, -2.0);

        /* reset glfw timer */
        glfwSetTime(0.0);

        /* compute new values */
        glDrawArrays(GL_POINTS, 0, 1);

        /* bind default framebuffer */
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        /* swap particle textures */
        swapparticletex(particletex, particletex2, particlefbo);

        /* handle events */
        glfwPollEvents();
    
    }

    /* destroy & deinit */
    glfwDestroyWindow(window);
    glfwTerminate();
    
}
