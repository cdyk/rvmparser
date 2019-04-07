#include <cassert>
#include <cstdio>
#include <string>
#include <algorithm>
#include "ExportSCAD.h"
#include "Store.h"
#include "LinAlgOps.h"

namespace {

  bool open_w(FILE** f, const char* path)
  {
    auto err = fopen_s(f, path, "w");
    if (err == 0) return true;

    char buf[1024];
    if (strerror_s(buf, sizeof(buf), err) != 0) {
      buf[0] = '\0';
    }
    fprintf(stderr, "Failed to open %s for writing: %s", path, buf);
    return false;
  }

  void wireBoundingBox(std::vector<float> &verts, unsigned& off_v, const BBox3f& bbox)
  {
    for (unsigned i = 0; i < 8; i++) {
      float px = (i & 1) ? bbox.min[0] : bbox.min[3];
      float py = (i & 2) ? bbox.min[1] : bbox.min[4];
      float pz = (i & 4) ? bbox.min[2] : bbox.min[5];
      verts.push_back(px);
    }
    off_v += 8;
  }

  void getMidpoint(Vec3f& p, Geometry* geo)
  {
    switch (geo->kind) {
    case Geometry::Kind::CircularTorus: {
      auto & ct = geo->circularTorus;
      auto c = std::cos(0.5f * ct.angle);
      auto s = std::sin(0.5f * ct.angle);
      p.x = ct.offset * c;
      p.y = ct.offset * s;
      p.z = 0.f;
      break;
    }
    default:
      p = Vec3f(0.f);
      break;
    }
    p = mul(geo->M_3x4, p);
  }
  
  char * mat2string(Mat3x4f m){
    char res[1024];
    // sprintf(res,"m=[[%f,%f,%f,%f],[%f,%f,%f,%f],[%f,%f,%f,%f],[0,0,0,1]]",
    // m.m01,m.m02,m.m03,
    // m.m11,m.m12,m.m13,
    // m.m21,m.m22,m.m23);
    sprintf(res,"m=[[%f,%f,%f,%f],[%f,%f,%f,%f],[%f,%f,%f,%f],[0,0,0,1]]",
    m.m00,m.m01,m.m02,m.m03,
    m.m10,m.m11,m.m12,m.m13,
    m.m20,m.m21,m.m22,m.m23);
    return res;
  }
  
}



ExportSCAD::~ExportSCAD()
{
  if (out) {
    fclose(out);
  }
}

bool ExportSCAD::open(const char* path_obj)
{
  if (!open_w(&out, path_obj)) return false;
  return true;
}


void ExportSCAD::init(class Store& store)
{
  assert(out);
  this->store = &store;
  conn = store.conn;

  stack.accommodate(store.groupCountAllocated());

  char colorName[6];
  for (auto * line = store.getFirstDebugLine(); line != nullptr; line = line->next) {

    verts.push_back(line->a[0]);
    verts.push_back(line->a[1]);
    verts.push_back(line->a[2]);
    verts.push_back(line->b[0]);
    verts.push_back(line->b[1]);
    verts.push_back(line->b[2]);
    off_v += 2;
  }
}

void ExportSCAD::beginFile(Group* group)
{
  //fprintf(out, "OFF\n");
}

void ExportSCAD::endFile() {
      FILE* m_out;
}

void ExportSCAD::beginModel(Group* group)
{
  //printf( "# Model project=%s, name=%s\n", group->model.project, group->model.name);
  fprintf(out,"union(){\n");
}

void ExportSCAD::endModel() {
  fprintf(out,"}\n");
}

void ExportSCAD::beginGroup(Group* group)
{
  //printf("%s\n",group->group.name);
  for (unsigned i = 0; i < 3; i++) curr_translation[i] = group->group.translation[i];

  stack[stack_p++] = group->group.name;

  if (groupBoundingBoxes && !isEmpty(group->group.bboxWorld)) {
    wireBoundingBox(verts, off_v, group->group.bboxWorld);
  }
  //fprintf(out,"translate([%f,%f,%f]){\n",group->group.translation[0],group->group.translation[1],group->group.translation[2]);
  
}

void ExportSCAD::EndGroup() {
  assert(stack_p);
  stack_p--;
  //fprintf(out,"}\n");
}

void ExportSCAD::geometry(struct Geometry* geometry)
{
  const auto & M = geometry->M_3x4;
  // printf("%d\n",geometry->kind);
  std::vector<float> c_verts;
  std::vector<uint32_t> c_tris;
  
  switch(geometry->kind){
    case Geometry::Kind::Pyramid:
      break;
    case Geometry::Kind::Box:{
      fprintf(out,"multmatrix(%s){cube([%f,%f,%f],center=true);}\n",mat2string(geometry->M_3x4),
      geometry->box.lengths[0],geometry->box.lengths[1],geometry->box.lengths[2]);
      break;
    }
    case Geometry::Kind::RectangularTorus:
      break;
    case Geometry::Kind::CircularTorus:
      break;
    case Geometry::Kind::EllipticalDish:
      break;
    case Geometry::Kind::SphericalDish:
      break;
    case Geometry::Kind::Snout:{
      fprintf(out,"multmatrix(%s){cylinder(h=%f,r1=%f,r2=%f,center=true,$fn=50);}\n",mat2string(geometry->M_3x4),
      geometry->snout.height,geometry->snout.radius_b,geometry->snout.radius_t);
      break;
    }
    case Geometry::Kind::Cylinder:{
      fprintf(out,"multmatrix(%s){cylinder(h=%f,r=%f,center=true,$fn=50);}\n",mat2string(geometry->M_3x4),
      geometry->cylinder.height,geometry->cylinder.radius);
      break;
    }
    case Geometry::Kind::Sphere:
      break;
    case Geometry::Kind::Line:
      break;
    case Geometry::Kind::FacetGroup:{
      Triangulation tri=*geometry->triangulation;
      printf("tris:%d,verts:%d\n",tri.triangles_n,tri.vertices_n);
      fprintf(out,"multmatrix(%s){\npolyhedron(points=[",mat2string(geometry->M_3x4));
      
      for(int i=0;i<3*tri.vertices_n-3;i+=3){
        fprintf(out,"[%f,%f,%f],\n",tri.vertices[i+0],tri.vertices[i+1],tri.vertices[i+2]);
      }
      fprintf(out,"[%f,%f,%f]],\nfaces=[",tri.vertices[3*tri.vertices_n-3],tri.vertices[3*tri.vertices_n-2],tri.vertices[3*tri.vertices_n-1]);
      
      for(int i=0;i<3*tri.triangles_n-3;i+=3){
        fprintf(out,"[%d,%d,%d],\n",tri.indices[i+0],tri.indices[i+1],tri.indices[i+2]);
      }
      fprintf(out,"[%d,%d,%d]],convexity=10);}\n",tri.indices[3*tri.triangles_n-3],tri.indices[3*tri.triangles_n-2],tri.indices[3*tri.triangles_n-1]);
      break;
    }
    default:
      break;
  }
  
  if (geometry->colorName == nullptr) {
    geometry->colorName = store->strings.intern("default");
  }

  auto scale = 1.f;
  
  if (geometry->kind == Geometry::Kind::Line) {
    auto a = scale * mul(geometry->M_3x4, Vec3f(geometry->line.a, 0, 0));
    auto b = scale * mul(geometry->M_3x4, Vec3f(geometry->line.b, 0, 0));
    verts.push_back(a.x);
    verts.push_back(a.y);
    verts.push_back(a.z);
    verts.push_back(b.x);
    verts.push_back(b.y);
    verts.push_back(b.z);
    off_v += 2;
  }
  else {
    assert(geometry->triangulation);
    auto * tri = geometry->triangulation;

    if (tri->indices != 0) {
      //sprintf(buffer, "g\n");
      if (geometry->triangulation->error != 0.f) {
        printf("# error=%f\n", geometry->triangulation->error);
      }
      for (size_t i = 0; i < 3 * tri->vertices_n; i += 3) {

        auto p = scale * mul(geometry->M_3x4, Vec3f(tri->vertices + i));
        auto n = normalize(mul(Mat3f(geometry->M_3x4.data), Vec3f(tri->normals + i)));

        verts.push_back(p.x);
        verts.push_back(p.y);
        verts.push_back(p.z);
        c_verts.push_back(p.x);
        c_verts.push_back(p.y);
        c_verts.push_back(p.z);
      }
      if (tri->texCoords) {
        for (size_t i = 0; i < tri->vertices_n; i++) {
          const Vec2f vt(tri->texCoords + 2 * i);
        }
      }
      else {
        for (size_t i = 0; i < tri->vertices_n; i++) {
          auto p = scale * mul(geometry->M_3x4, Vec3f(tri->vertices + 3*i));
        }

        for (size_t i = 0; i < 3 * tri->triangles_n; i += 3) {
          auto a = tri->indices[i + 0];
          auto b = tri->indices[i + 1];
          auto c = tri->indices[i + 2];
          tris.push_back(a + off_v - 1);
          tris.push_back(b + off_v - 1);
          tris.push_back(c + off_v - 1);
          c_tris.push_back(a);
          c_tris.push_back(b);
          c_tris.push_back(c);
        }
      }

      off_v += tri->vertices_n;
      off_n += tri->vertices_n;
      off_t += tri->vertices_n;
    }
  }

  //if (primitiveBoundingBoxes) {
  //  sprintf(buffer, "usemtl magenta\n");

  //  for (unsigned i = 0; i < 8; i++) {
  //    float px = (i & 1) ? geometry->bbox[0] : geometry->bbox[3];
  //    float py = (i & 2) ? geometry->bbox[1] : geometry->bbox[4];
  //    float pz = (i & 4) ? geometry->bbox[2] : geometry->bbox[5];

  //    float Px = M[0] * px + M[3] * py + M[6] * pz + M[9];
  //    float Py = M[1] * px + M[4] * py + M[7] * pz + M[10];
  //    float Pz = M[2] * px + M[5] * py + M[8] * pz + M[11];

  //    sprintf(buffer, "v %f %f %f\n", Px, Py, Pz);
  //  }
  //  sprintf(buffer, "l %d %d %d %d %d\n",
  //          off_v + 0, off_v + 1, off_v + 3, off_v + 2, off_v + 0);
  //  sprintf(buffer, "l %d %d %d %d %d\n",
  //          off_v + 4, off_v + 5, off_v + 7, off_v + 6, off_v + 4);
  //  sprintf(buffer, "l %d %d\n", off_v + 0, off_v + 4);
  //  sprintf(buffer, "l %d %d\n", off_v + 1, off_v + 5);
  //  sprintf(buffer, "l %d %d\n", off_v + 2, off_v + 6);
  //  sprintf(buffer, "l %d %d\n", off_v + 3, off_v + 7);
  //  off_v += 8;
  //}


  //for (unsigned k = 0; k < 6; k++) {
  //  auto other = geometry->conn_geo[k];
  //  if (geometry < other) {
  //    sprintf(buffer, "usemtl blue_line\n");
  //    float p[3];
  //    getMidpoint(p, geometry);
  //    sprintf(buffer, "v %f %f %f\n", p[0], p[1], p[2]);
  //    getMidpoint(p, other);
  //    sprintf(buffer, "v %f %f %f\n", p[0], p[1], p[2]);
  //    sprintf(buffer, "l %d %d\n", off_v, off_v + 1);

  //    off_v += 2;
  //  }
  //}

}
