#!/usr/bin/env python3

# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2020 Metrological
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import argparse
import sys
import os
import glob
import subprocess
import shutil
from git import Repo
import time


THUNDER_REPO_URL = "https://github.com/rdkcentral/Thunder"
THUNDER_INTERFACE_REPO_URL = "https://github.com/rdkcentral/ThunderInterfaces"
THUNDER_PLUGINS_REPO_URL = "https://github.com/rdkcentral/ThunderNanoServices.git"
RDK_PLUGINS_REPO_URL = "https://github.com/WebPlatformForEmbedded/ThunderNanoServicesRDK.git"
DOCS_REPO_URL = "https://github.com/WebPlatformForEmbedded/ServicesInterfaceDocumentation.git"
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir))
import JsonGenerator.JsonGenerator as JsonGenerator
import ProxyStubGenerator.Log as Log

NAME = "DocumentGenerator"
VERBOSE = True
SHOW_WARNINGS = True
DOC_ISSUES = False

log = Log.Log(NAME, VERBOSE, SHOW_WARNINGS, DOC_ISSUES)

class MkdocsYamlFileGenerator():
    def __init__(self, docs_path, site_name, site_url):
        self._yamlfile_path = docs_path + "mkdocs.yml"
        self._site_name = site_name
        self._site_url = site_url
        self._current_topic = ""
        self._topic_dict = {}
        self._fd = None
        return

    def create_site_details(self, site_name, site_url):
        assert self._fd != None
        self._fd.write("site_name : " + site_name +"\n")
        self._fd.write("site_url : " + site_url +"\n")
        return

    def add_nav_tag(self):
        assert self._fd != None
        self._fd.write("nav:\n    - 'Documentation': 'index.md'\n")
        return


    def add_topic(self, topic_name):
        assert self._fd != None
        self._fd.write("    - '" + topic_name + "':\n")
        return

    def add_subtopic(self, sub_topic_name, markdown_filename):
        assert self._fd != None
        self._fd.write("        - '" + sub_topic_name + "' : '" + markdown_filename +"'\n")
        return

    def create_topics(self, topic_name):
        if topic_name not in self._topic_dict.keys():
            self._topic_dict[topic_name] = []
        self._current_topic = topic_name
        return

    def create_subtopics(self, sub_topic_name, markdown_filename):
        if self._topic_dict[self._current_topic] != None and isinstance(self._topic_dict[self._current_topic], list):
            self._topic_dict[self._current_topic].append((sub_topic_name, markdown_filename))
        return
    
    def create_file(self):
        self._fd = open(self._yamlfile_path, "w")
        self.create_site_details(self._site_name, self._site_url)
        self.add_theme_info()
        self.add_nav_tag()
        for topic in self._topic_dict:
            self.add_topic(topic)
            self._topic_dict[topic].sort()
            for subtopic in self._topic_dict[topic]:
                self.add_subtopic(subtopic[0], subtopic[1])
        self._fd.close()
        self._fd = None
        return

    def add_theme_info(self):
        assert self._fd != None
        theme_info = '''theme:
    name: material
    highlightjs: true
markdown_extensions:
    - pymdownx.emoji:
        emoji_index: !!python/name:materialx.emoji.twemoji
        emoji_generator: !!python/name:materialx.emoji.to_svg'''

        self._fd.write(theme_info + "\n")
        return

class DocumentGenerator():
    _yaml_generator = None
    def __init__(self, thunder_path, thunder_interface_path, thunder_plugins_path, rdk_plugins_path, docs_path):
        self.thunder_path = thunder_path
        self.thunder_interface_path = thunder_interface_path
        self.thunder_plugins_path = thunder_plugins_path
        self.rdk_plugins_path = rdk_plugins_path
        self.docs_path = docs_path
        self.clean_all_repos_dir()
        self.clone_all_repos()
        self._yaml_generator = MkdocsYamlFileGenerator(self.docs_path, "Documentation", "")
        self.mkdocs_create_index_file()

    def clean_all_repos_dir(self):
        if os.path.exists(self.thunder_path):
            shutil.rmtree(self.thunder_path)
        if os.path.exists(self.thunder_interface_path):
            shutil.rmtree(self.thunder_interface_path)
        if os.path.exists(self.thunder_plugins_path):
            shutil.rmtree(self.thunder_plugins_path)
        if os.path.exists(self.rdk_plugins_path):
            shutil.rmtree(self.rdk_plugins_path)
        if os.path.exists(self.docs_path):
            shutil.rmtree(self.docs_path)
        return

    def clone_all_repos(self):
        self.thunder_commit_id, self.thunder_commit_date = self.clone_repo(THUNDER_REPO_URL, self.thunder_path)
        self.thunder_interfaces_commit_id, self.thunder_interfaces_commit_date = self.clone_repo(THUNDER_INTERFACE_REPO_URL, self.thunder_interface_path)
        self.thunder_plugins_commit_id, self.thunder_plugins_commit_date = self.clone_repo(THUNDER_PLUGINS_REPO_URL, self.thunder_plugins_path)
        self.rdk_plugins_commit_id, self.rdk_plugins_commit_date = self.clone_repo(RDK_PLUGINS_REPO_URL, self.rdk_plugins_path)
        self.docs_commit_id, self.docs_commit_date = self.clone_repo(DOCS_REPO_URL, self.docs_path)


    def clone_repo(self, repo_url, local_path):
        repo = Repo.clone_from(repo_url, local_path)
        headcommit = repo.head.commit
        headcommit_sha = headcommit.hexsha
        headcommit_date = time.strftime("%a, %d %b %Y %H:%M", time.gmtime(headcommit.committed_date))
        log.Info("Repo head: {} ".format( headcommit.hexsha))
        log.Info("Repo commit date: {}".format( headcommit_date))
        return (headcommit_sha, headcommit_date)

    def mkdocs_create_index_file(self):
        if not os.path.exists(self.docs_path + "/docs"):
            os.mkdir(self.docs_path + "/docs")

        index_file = open(self.docs_path + "/docs/index.md", "w")
        index_file_interface_contents = "# Welcome to Documentation.\nThese documentation are automatically created using mkdocs on " + time.strftime("%a, %d %b %Y %H:%M", time.gmtime()) + " GMT\n\
## Interface documentation\nThis section contains the documentation created from interfaces\n\n\
| Repo | Commit-Id | Commit-Date |\n\
| :--- | :-------- | :---------- |\n\
|[ThunderInterfaces](" + THUNDER_INTERFACE_REPO_URL + ')|' + self.thunder_interfaces_commit_id + '|' + self.thunder_interfaces_commit_date + " GMT|\n\n"
        index_file_contents_plugins = '''## WPEFramework-plugins documentation
This section contains the documentation created from plugins\n\n
| Repo | Commit-Id | Commit-Date |
| :--- | :-------- | :---------- |
| [ThunderNanoServices]('''+THUNDER_PLUGINS_REPO_URL + ') | '  + self.thunder_plugins_commit_id + ' | ' + self.thunder_plugins_commit_date + " GMT |\n| [ThunderNanoServicesRDK]("+ RDK_PLUGINS_REPO_URL + ') | '  + self.rdk_plugins_commit_id + ' | ' + self.rdk_plugins_commit_date + " GMT |\n"

        index_file.write(index_file_interface_contents)
        index_file.write(index_file_contents_plugins)

        return

    def generate_document(self, schemas):
        # output_path = "/Users/ksomas586/testPrgs/MarkDown/documentation/out/"
        output_path = self.docs_path + "/docs/"
        for schema in schemas:
            if schema:
                warnings = JsonGenerator.GENERATED_JSON
                JsonGenerator.GENERATED_JSON = "dorpc" in schema
                title = schema["info"]["title"] if "title" in schema["info"] \
                                    else schema["info"]["class"] if "class" in schema["info"] \
                                    else os.path.basename(output_path)
                try:
                    path = os.path.join(os.path.dirname(output_path), title.replace(" ", ""))
                    filename = os.path.basename(os.path.dirname(path) + "/" + os.path.basename(path).replace(".json", "") + ".md")
                    self._yaml_generator.create_subtopics(title, filename)
                    JsonGenerator.CreateDocument(schema, path)
                    JsonGenerator.GENERATED_JSON = warnings
                except RuntimeError as err:
                    log.Error("Error : {}".format( str(err)))
                except Exception as err:
                    log.Error("Error : {}".format( str(err)))

    def add_topic(self, topic_name):
        self._yaml_generator.create_topics(topic_name)
        return

    def header_to_markdown(self, path):
        for file in glob.glob(path):
            schemas = JsonGenerator.LoadInterface(file, [self.thunder_path + "/Source/"])
            self.generate_document(schemas)
        return

    def json_to_markdown(self, path):
        for file in glob.glob(path):
            if file.endswith("common.json"):
                continue
            schemas = [JsonGenerator.LoadSchema(file, thunder_interface_path + "/jsonrpc/", thunder_interface_path + "/interfaces/", [thunder_path + "/Source/"])]
            self.generate_document(schemas)
        return
    
    def complete_yaml_creation(self):
        self._yaml_generator.create_file()

    #def __del__(self):
    #    self.clean_all_repos_dir()

if __name__ == "__main__":
    argparser = argparse.ArgumentParser(
        description='Generate API and plugin documentation.',
        formatter_class=argparse.RawTextHelpFormatter)
    docs_repo_url = os.environ.get("DOCS_REPO_URL")
    if docs_repo_url != None:
        DOCS_REPO_URL = docs_repo_url
    argparser.add_argument("--deploy",
                            dest="github_deploy",
                            action="store_true",
                            default=False,
                            help="Deploy generated code in github")
    argparser.add_argument("--clone_path",
                            dest="clone_path",
                            action="store",
                            type=str,
                            metavar="DIR",
                            required=True,
                            help="Local path for cloning necessary repositories")

    args = argparser.parse_args(sys.argv[1:])
    clone_path = args.clone_path
    deploy_docs = args.github_deploy
    thunder_interface_path = clone_path + "/interface/"
    thunder_plugins_path = clone_path + "/thunder_nano_services/"
    rdk_plugins_path = clone_path + "/thunder_nano_services_rdk/"
    thunder_path = clone_path + "/thunder/"
    docs_path = clone_path + "/Documentation/"
    log.Info("Interface path: {} ".format( thunder_interface_path))
    log.Info("Plugin path: {}".format( thunder_plugins_path))
    log.Info("RDKPlugin path: {}".format( rdk_plugins_path))
    log.Info("Thunder path: {}".format( thunder_path))
    log.Info("Documentation path: {}".format( docs_path))


    document_generator = DocumentGenerator(thunder_path, thunder_interface_path, thunder_plugins_path, rdk_plugins_path, docs_path)
    os.chdir(thunder_path + "/Tools/JsonGenerator/")

    log.Info("Adding Interface Documentation")
    document_generator.add_topic("Interface Documentation")
    document_generator.header_to_markdown(thunder_interface_path + "/interfaces/*.h")
    document_generator.json_to_markdown(thunder_interface_path + "/jsonrpc/*.json")
    log.Info("Adding Plugin Documentation")
    document_generator.add_topic("Plugin Documentation")
    document_generator.json_to_markdown(thunder_plugins_path + "/*/*Plugin.json")
    document_generator.json_to_markdown(rdk_plugins_path + "/*/*Plugin.json")
    document_generator.complete_yaml_creation()
    if deploy_docs:
        # unless DOCS_REPO_URL is changed the documentation will be deployed in this location https://webplatformforembedded.github.io/ServicesInterfaceDocumentation/
        os.chdir(docs_path)
        try:

            ret_val = subprocess.run(["mkdocs", "gh-deploy"])
            log.Info("mkdocs exit code: {} ".format( ret_val.returncode))
        except subprocess.CalledProcessError as err:
            log.Error("Error in deploying: {}".format( str(err)))
    document_generator.clean_all_repos_dir()


